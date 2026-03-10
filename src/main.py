import rules
import random
import os

random.seed(4)

x=rules.GameState()
i=0

while(x.is_terminal()==False):
    move=random.choice(x.get_legal_actions())
    x=x.apply_action(move)
    print("STATE",i)
    x.__repr__()
    i+=1
    # os.system('cls' if os.name == 'nt' else 'clear')