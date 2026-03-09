import rules
import random
import os

random.seed(1)

x=rules.GameState()

for i in range(1000):
    move=random.choice(x.get_legal_actions())
    x=x.apply_action(move)
    print("STAGE",i)
    print(x)
    # os.system('cls' if os.name == 'nt' else 'clear')