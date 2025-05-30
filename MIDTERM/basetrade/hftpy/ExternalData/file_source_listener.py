from abc import abstractmethod


class FileSourceListener:

    def __init__(self):
        pass

    @abstractmethod
    def process_all_events(self):
        return

    @abstractmethod
    def process_events_till(self):
        return

    @abstractmethod
    def seek_to_first_event_after(self, _start_time_):
        return
