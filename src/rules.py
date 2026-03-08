WID=15
HEI=19

def norm_pos(pos):
    y,x=pos
    while(y<0):
        y+=HEI
    while(y>HEI):
        y-=HEI
    while(x<0):
        x+=WID
    while(x>WID):
        x-=WID
    return y,x

class GameState:
    def __init__(
        self,
        board=[None]*WID*HEI,
        current_player=0,
        legal_actions_O=set([
            (4, 10),
            (3, 11),
            (5, 11),
            (4, 12)
        ]),
        legal_actions_X=set([
            (14,2),
            (13,3),
            (15,3),
            (14,4)
        ]),
        state=(0,-1)
    ):
        self.current_player=current_player
        self.board=board
        self.legal_actions_O=legal_actions_O
        self.legal_actions_X=legal_actions_X
        self.state=state
    def get_legal_actions(self)->list:
        if(self.current_player):
            return list(self.legal_actions_X)
        return list(self.legal_actions_O)
    # ↓ TODO
    # 1. debug
    # 2. draw examine
    def apply_action(self, action)->'GameState':
        directions=[
            (0,1), (0,-1),
            (1,0), (-1,0)
        ]
        board=self.board[:]
        def cnt_suc(
                start_pos, direction, target
            ):
            cnt=0
            y,x=start_pos
            while(board[y*WID+x]==target):
                cnt+=1
                y+=direction[0]
                x+=direction[1]
            return cnt
        def apply_remove(removed_pos, player):
            board[
                removed_pos[0]*WID
                +removed_pos[1]
            ]=None
            for direction in directions:
                y,x=removed_pos
                y+=direction[0],
                x+=direction[1]
                suc_cnt=cnt_suc(
                    (y,x), direction,
                    player
                )
                y+=suc_cnt*direction[0]
                x+=suc_cnt*direction[1]
                if(suc_cnt==0):
                    continue
                if(board[y*WID+x]!=None):
                    enemy_cnt=cnt_suc(
                        (y,x), direction,
                        1-player
                    )
                    if(suc_cnt<enemy_cnt):
                        y,x=removed_pos
                        y+=direction[0]
                        x+=direction[1]
                        for _ in range(suc_cnt):
                            apply_remove(
                                (y,x), player)
                            y+=direction[0]
                            x+=direction[1]
        lenths_each_direction=[
            cnt_suc(
                action, direction,
                self.current_player
            )
            for direction in directions
        ]
        lenth_x=(lenths_each_direction[0]+
            lenths_each_direction[1]-1)
        lenth_y=(lenths_each_direction[2]+
            lenths_each_direction[3]-1)
        for i,lenth in enumerate(
            lenths_each_direction
        ):
            direction=directions[i]
            y=action[0]+direction[0]*lenth
            x=action[1]+direction[1]*lenth
            if(board[y*WID+x]!=None):
                enemy_cnt=cnt_suc(
                    (y,x), direction,
                    1-self.current_player
                )
                total_lenth=(
                    lenth_x if
                    (i==0 or i==1) else
                    lenth_y
                )
                if(enemy_cnt<total_lenth):
                    for i in range(enemy_cnt):
                        apply_remove((y,x), 1-
                            self.current_player)
                        y+=direction[0]
                        x+=direction[1]
        if(board[4*WID+11]==None):
            return GameState(
                board, 1-self.current_player,
                set(), set(), (1,1)
            )
        if(board[14*WID+3]==None):
            return GameState(
                board, 1-self.current_player,
                set(), set(), (1,0)
            )
        legal_actions_O=(
            self.legal_actions_O.copy())
        legal_actions_X=(
            self.legal_actions_X.copy())
        for direction in directions:
            new_y=action[0]+direction[0]
            new_x=action[1]+direction[1]
            if(
                board[new_y*WID+new_x]!=None
            ):
                continue
            flag=True
            for exam_dir in directions:
                friend_cnt=cnt_suc(
                    (
                        new_y+exam_dir[0],
                        new_x+exam_dir[1]
                    ),
                    exam_dir,
                    self.current_player
                )
                enemy_cnt=cnt_suc(
                    (
                        new_x-exam_dir[0],
                        new_y-exam_dir[1]
                    ),
                    (-exam_dir[0], -exam_dir[1]),
                    1-self.current_player
                )
                if(friend_cnt+1<enemy_cnt):
                    flag=False
                    break
            if(flag):
                legal_actions=(
                    legal_actions_X if
                    self.current_player
                    else legal_actions_O
                )
                legal_actions.add((new_y, new_x))
        legal_actions.remove((new_y, new_x))
        return GameState(
            board, 1-self.current_player,
            legal_actions_O, legal_actions_X,
            (0,-1)
        )
    def get_current_player(self)->int:
        return self.current_player
    def is_terminal(self)->bool:
        return self.state[0]
    def get_reward(self, player_id: int)->float:
        if(self.state[1]==-1):
            return 0.0
        if(player_id==self.current_player):
            return 1.0
        return -1.0
    def __hash__(self):
        pass
    def __eq__(self, other):
        pass
    def __repr__(self):
        pass
