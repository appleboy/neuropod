#
# Uber, Inc. (c) 2018
#

import numpy as np
import os
import tensorflow as tf
import unittest
from testpath.tempdir import TemporaryDirectory

from neuropods.backends.tensorflow.trt import is_trt_available
from neuropods.packagers import create_tensorflow_neuropod
from neuropods.loader import load_neuropod
from neuropods.tests.utils import get_addition_model_spec, check_addition_model

def create_tf_addition_model():
    """
    A simple addition model
    """
    g = tf.Graph()
    with g.as_default():
        with tf.name_scope("some_namespace"):
            x = tf.placeholder(tf.float32, name="in_x", shape=(None,))
            y = tf.placeholder(tf.float32, name="in_y", shape=(None,))

            # UATG(flake8/F841) Assigned to a variable for clarity
            out = tf.add(x, y, name="out")

    return g.as_graph_def()


def create_tf_accumulator_model():
    """
    Accumulate input x into a variable. Return the accumulated value.
    """
    g = tf.Graph()
    with g.as_default():
        with tf.name_scope("some_namespace"):
            acc = tf.get_variable('accumulator', initializer=tf.zeros_initializer(), shape=(), dtype=tf.float32)
            x = tf.placeholder(tf.float32, name="in_x")

            assign_op = tf.assign_add(acc, x)
            with tf.control_dependencies([assign_op]):
                tf.identity(acc, name="out")
        init_op = tf.global_variables_initializer()

    return g.as_graph_def(), init_op.name


class TestTensorflowPackaging(unittest.TestCase):
    def package_simple_addition_model(self, do_fail=False, use_trt=False):
        with TemporaryDirectory() as test_dir:
            neuropod_path = os.path.join(test_dir, "test_neuropod")

            # `create_tensorflow_neuropod` runs inference with the test data immediately
            # after creating the neuropod. Raises a ValueError if the model output
            # does not match the expected output.
            create_tensorflow_neuropod(
                neuropod_path=neuropod_path,
                model_name="addition_model",
                graph_def=create_tf_addition_model(),
                node_name_mapping={
                    "x": "some_namespace/in_x:0",
                    "y": "some_namespace/in_y:0",
                    "out": "some_namespace/out:0",
                },
                use_trt=use_trt,
                # Get the input/output spec along with test data
                **get_addition_model_spec(do_fail=do_fail)
            )

            # Run some additional checks
            check_addition_model(neuropod_path)

    def package_accumulator_model(self, neuropod_path, init_op_name_as_list, use_trt=False):
        graph_def, init_op_name = create_tf_accumulator_model()

        # `create_tensorflow_neuropod` runs inference with the test data immediately
        # after creating the neuropod. Raises a ValueError if the model output
        # does not match the expected output.
        create_tensorflow_neuropod(
            neuropod_path=neuropod_path,
            model_name="accumulator_model",
            graph_def=graph_def,
            node_name_mapping={
                "x": "some_namespace/in_x:0",
                "out": "some_namespace/out:0",
            },
            input_spec=[
                {"name": "x", "dtype": "float32", "shape": ()},
            ],
            output_spec=[
                {"name": "out", "dtype": "float32", "shape": ()},
            ],
            init_op_names=[init_op_name] if init_op_name_as_list else init_op_name,
            test_input_data={
                "x": np.float32(5.0),
            },
            test_expected_out={
                "out": np.float32(5.0),
            },
            use_trt=use_trt,
        )

    def test_simple_addition_model(self):
        # Tests a case where packaging works correctly and
        # the model output matches the expected output
        self.package_simple_addition_model()

    def test_simple_addition_model_failure(self):
        # Tests a case where the output does not match the expected output
        with self.assertRaises(ValueError):
            self.package_simple_addition_model(do_fail=True)

    def validate_stateful_model(self, use_trt):
        # `init_op` can be passed a list of strings or a string
        for init_op_name_as_list in [False, True]:
            with TemporaryDirectory() as test_dir:
                neuropod_path = os.path.join(test_dir, "test_neuropod")
                self.package_accumulator_model(neuropod_path, init_op_name_as_list, use_trt=use_trt)
                neuropod_path = load_neuropod(neuropod_path)
                np.testing.assert_equal(neuropod_path.infer({"x": np.float32(2.0)}), {"out": 2.0})
                np.testing.assert_equal(neuropod_path.infer({"x": np.float32(4.0)}), {"out": 6.0})

    def test_stateful_model(self):
        # Tests a stateful model
        self.validate_stateful_model(use_trt=False)

    @unittest.skipIf(not is_trt_available(), "TRT is not available in this version of TF")
    def test_stateful_model_trt(self):
        # Tests a stateful model using TRT
        self.validate_stateful_model(use_trt=True)

    @unittest.skipIf(not is_trt_available(), "TRT is not available in this version of TF")
    def test_simple_addition_model_trt(self):
        # Tests TRT optimization
        self.package_simple_addition_model(use_trt=True)


if __name__ == '__main__':
    unittest.main()
