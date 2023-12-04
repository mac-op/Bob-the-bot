import os
import random

os.chdir("build/bin")

MAPS = ["CactusValleyLE.SC2Map", "BelShirVestigeLE.SC2Map", "ProximaStationLE.SC2Map"]
DIFFICULTIES = ["Medium", "MediumHard", "Hard"]
RACES = ["terran", "protoss", "zerg"]
N = 20

class TestCase:
    def __init__(self, map_, difficulty_, race_):
        self.map = map_
        self.difficulty = difficulty_
        self.race = race_
        self.win = False

    def __repr__(self):
        return f"Map: {self.map} - Difficulty: {self.difficulty} - " \
               f"Race: {self.race} - {'Won' if self.win else 'Lost'}\n"


if __name__ == "__main__":
    for difficulty in DIFFICULTIES:
        total = 0
        wins = 0
        for _ in range(N):
            game_map = random.choice(MAPS)
            race = random.choice(RACES)
            tc = TestCase(game_map, difficulty, race)

            f = os.popen(f"BobTheBot.exe -c -a {race} -d {difficulty} -m {game_map}")
            lines = f.readlines()
            status = f.close()
            if not status:
                total += 1
                if "0\n" in lines:
                    tc.win = True
                    wins += 1
                print(tc)
        print(f"{difficulty}: {wins}/{total} wins")
