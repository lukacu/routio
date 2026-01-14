
import unittest

from routio import Client, IOLoop

class Tests(unittest.TestCase):

    def test_tensor(self):
        from routio.array import TensorPublisher, TensorSubscriber

        client = Client()