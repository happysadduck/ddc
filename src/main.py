import rules
import random

random.seed(1)

x=rules.GameState()
print(x)

for i in range(200):
    if(i==57):
        pass
    move=random.choice(x.get_legal_actions())
    x=x.apply_action(move)
    print("STAGE",i)
    print(x)