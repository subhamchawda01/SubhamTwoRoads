from abc import abstractmethod


class OrderExecListener:

    def __init__(self):
        pass

    @abstractmethod
    def on_exec(self):
        pass
