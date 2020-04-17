
import zmq
import json

class Reporter:
    def __init__(self, report_name):
        context = zmq.Context()

        self.socket = context.socket(zmq.REQ)
        self.socket.connect("tcp://localhost:5555")
        self.name = report_name

    def report(self, description):
        self.socket.send("{}".format(description).encode())
        #  Get the reply.
        msg = self.socket.recv()


