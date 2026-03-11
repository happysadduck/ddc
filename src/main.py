import rules
import random

seed=0
i=10000

while i>=100:
    i=0
    x=rules.GameState()
    random.seed(seed)
    while(x.is_terminal()==False):
        move=random.choice(x.get_legal_actions())
        x=x.apply_action(move)
        i+=1
    print(i, seed)
    seed+=1

# x=rules.GameState()
# random.seed(17682)
# i=0
# while(x.is_terminal()==False):
#     move=random.choice(x.get_legal_actions())
#     x=x.apply_action(move)
#     i+=1
#     print("STATE",i)
#     print(x.__repr__())