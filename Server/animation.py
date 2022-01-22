import time


class AnimationLoop:
    """
    This class plays sequence of animation commands from the config.json file.
    """
    def __init__(self):
        self.animation = None
        self.start_time = None
        self.timeout = 0
        self.reset()

    def reset(self):
        self.animation_index = 0
        self.next_animation = time.time()

    def start(self, animation, timeout=0):
        """
        Start the given animation with a maximum timeout in seconds.
        If timeout is 0 it could run forever, depending on the animation.
        """
        print("### playing animation: {}".format(animation["Name"]))
        self.animation = animation
        self.animation_index = 0
        self.next_animation = time.time()
        self.start_time = time.time()
        self.timeout = timeout

    def ready(self):
        """
        Return true if the animation is ready to play the next step.
        """
        return time.time() > self.next_animation

    def completed(self):
        """
        Returns true if we've reached the timeout given to the start method.
        If the timeout was 0 it always returns false.
        """
        return self.timeout != 0 and time.time() > self.start_time + self.timeout

    def next(self):
        """
        Move to the next step in the animation sequence.
        You should call this when 'ready' has returned True.
        This method returns the list of commands that need to
        be executed or None.
        """
        duration = 0
        result = None
        if self.animation is not None:
            commands = self.animation["Commands"]
            while self.animation_index < len(commands):
                c = commands[self.animation_index]
                self.animation_index += 1
                if result is None:
                    result = []
                result += [c]
                timeout = 0
                if "seconds" in c:
                    timeout = c["seconds"]
                if "hold" in c:
                    timeout += c["hold"]
                if timeout > duration:
                    duration = timeout
                if "start" in c and c["start"] == "after-previous":
                    break
            if self.animation_index == len(commands):
                if "repeat" in self.animation and self.animation["repeat"] == "forever":
                    self.animation_index = 0

        self.next_animation = time.time() + duration
        if result:
            print("Running animation '{}', step {} for {} seconds".format(self.animation['Name'], self.animation_index, duration))
        return result