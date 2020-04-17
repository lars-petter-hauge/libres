import time
import zmq

import json

class MessageServer:
    def __init__(self):
        context = zmq.Context()
        self.socket = context.socket(zmq.REP)
        self.socket.bind("tcp://*:5555")

    def run(self):
        while True:
            #  Wait for next request from client
            message = self.socket.recv()
            print(message)
            self.socket.send("msg received".encode())
