class HikeSession:
    id = 0
    m = 0
    steps = 0
    coords = []

    def __repr__(self):
        return f"HikeSession{{{self.id}, {self.m}(m), {self.steps}(steps)}}"

def to_list(s: HikeSession) -> list:
    return [s.id, s.m, s.steps]

def from_list(l: list) -> HikeSession:
    s = HikeSession()
    s.id = l[0]
    s.m = l[1]
    s.steps = l[2]
    return s