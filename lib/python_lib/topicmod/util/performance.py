
class PerformanceCalculator:
    def __init__(self):
        self.true_pos = 0
        self.p = 0.0
        self.a = 0.0
        self.t = 0.0
        self.r = 0.0

    def add_obs(self, guess, answer):
        if guess == 1 and answer == 1:
            self.true_pos += 1
        if guess == answer:
            self.r += 1

        if guess == 1:
            self.p += 1
        if answer == 1:
            self.a += 1
        self.t += 1
#    print guess, answer, self.true_pos, self.r, self.p, self.a

    def performance(self):
        precision = float(self.true_pos) / float(self.p)
        recall = float(self.true_pos) / float(self.a)
        accuracy = float(self.r) / float(self.t)

        f = 2 * precision * recall / (precision + recall)

        return precision, recall, f, accuracy
