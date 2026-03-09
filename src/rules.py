WID=15
HEI=19

def norm_pos(pos):
    y,x=pos
    while(y<0):
        y+=HEI
    while(y>=HEI):
        y-=HEI
    while(x<0):
        x+=WID
    while(x>=WID):
        x-=WID
    return y,x

class GameState:
    def __init__(
        self,
        board=None,
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
        state=(0,-1),
        last_action=None
    ):
        if(board==None):
            board=[None]*WID*HEI
            board[4*WID+11]=0
            board[14*WID+3]=1
        self.current_player=current_player
        self.board=board
        self.legal_actions_O=legal_actions_O
        self.legal_actions_X=legal_actions_X
        self.state=state
        self.last_action=last_action
    def get_legal_actions(self)->list:
        if(self.current_player):
            return list(self.legal_actions_X)
        return list(self.legal_actions_O)
    # ↓ TODO
    # 1. bugs in removing 
    # (not updating legal moves when removing)
    # 2. never stop loops
    # suggests:
    # s1. use tuple to store legality of pos
    # s2. reduce some code in apply_remove
    def apply_action(self, action)->'GameState':
        directions=[
            (0,1), (0,-1),
            (1,0), (-1,0)
        ]
        board=self.board[:]
        board[action[0]*WID+action[1]]=(
            self.current_player
        )
        def cnt_suc(
            start_pos, direction, target
        ):
            cnt=0
            y,x=start_pos
            while(board[y*WID+x]==target):
                cnt+=1
                y+=direction[0]
                x+=direction[1]
                y,x=norm_pos((y,x))
            return cnt
        def mv_suc(
            start_pos, direction, target
        ):
            y,x=start_pos
            lenth=cnt_suc(
                start_pos,direction,target
            )
            y+=direction[0]*lenth
            x+=direction[1]*lenth
            return norm_pos((y,x))
        def is_legal_pos(pos):
            y,x=pos
            if(board[y*WID+x]!=None):
                return None
            O_legal=True
            X_legal=True
            for direction in directions:
                O_cnt=cnt_suc(
                    norm_pos((y+direction[0],
                    x+direction[1])),
                    direction, 0
                )
                X_cnt=cnt_suc(
                    norm_pos((y-direction[0],
                     x-direction[1])),
                    (
                        -direction[0],
                        direction[1]
                    ), 1
                )
                if(O_cnt+1<X_cnt):
                    O_legal=False
                elif(O_cnt>X_cnt+1):
                    X_legal=False
            if(O_legal and X_legal):
                return 2
            if(O_legal):
                return 0
            if(X_legal):
                return 1
            return None
        def is_beside(pos):
            y,x=pos
            beside_O=False
            beside_X=False
            for direction in directions:
                y+=direction[0]
                x+=direction[1]
                y,x=norm_pos((t,x))
                if(board[y*WID+x]==0):
                    beside_O=True
                elif(board[y*WID+x]==1):
                    beside_X=True
            if(beside_O and beside_X):
                return 2
            if(beside_O):
                return 0
            if(beside_X):
                return 1
            return None
        def is_legal_pos_strict(pos):
            beside_info=is_beside(pos)
            legal_info=is_legal_pos(pos)
            if(beside_info==None):
                return None
            if(beside_info==1):
                if(legal_info==2 or
                    legal_info==1):
                    return 1
                return None
            if(beside_info==2):
                if(legal_info==2 or
                    legal_info==0):
                    return 0
                return None
            return legal_info
        def apply_remove(removed_pos, player):
            board[
                removed_pos[0]*WID
                +removed_pos[1]
            ]=None
            # TODO
            # each remove:
            # update itself & 4 pos beside it
            for direction in directions:
                y,x=removed_pos
                y+=direction[0]
                x+=direction[1]
                y,x=norm_pos((y,x))
                suc_cnt=cnt_suc(
                    (y,x), direction,
                    player
                )
                y+=suc_cnt*direction[0]
                x+=suc_cnt*direction[1]
                y,x=norm_pos((y,x))
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
                        y,x=norm_pos((y,x))
                        for _ in range(suc_cnt):
                            apply_remove(
                                (y,x), player)
                            y+=direction[0]
                            x+=direction[1]
                            y,x=norm_pos((y,x))
        for direction in directions:
            y,x=mv_suc(
                action, direction,
                self.current_player
            )
            if(board[y*WID+x]!=None):
                while(board[y*WID+x]==
                    1-self.current_player):
                    apply_remove((y,x), 1-
                        self.current_player)
                    y+=direction[0]
                    x+=direction[1]
                    y,x=norm_pos((y,x))
        if(board[4*WID+11]==None):
            return GameState(
                board, 1-self.current_player,
                set(), set(), (1,1), action
            )
        if(board[14*WID+3]==None):
            return GameState(
                board, 1-self.current_player,
                set(), set(), (1,0), action
            )
        legal_actions_O=(
            self.legal_actions_O.copy())
        legal_actions_X=(
            self.legal_actions_X.copy())
        legal_actions,enemy_actions=(
            (legal_actions_X,legal_actions_O) if
            self.current_player
            else (legal_actions_O,legal_actions_X)
        )
        for direction in directions:
            exm_y=action[0]+direction[0]
            exm_x=action[1]+direction[1]
            exm_y,exm_x=norm_pos((exm_y,exm_x))
            legal_state=is_legal_pos(
                (exm_y,exm_x)
            )
            if(legal_state==2):
                legal_actions.add((exm_y,exm_x))
            elif(legal_state==
                self.current_player):
                legal_actions.add((exm_y,exm_x))
                enemy_actions.discard(
                    (exm_y,exm_x)
                )
            elif(legal_state==None):
                enemy_actions.discard(
                    (exm_y,exm_x)
                )
        legal_actions.remove((action[0],action[1]))
        if((not legal_actions_O)
            and(not legal_actions_X)):
            return GameState(
                board, 1-self.current_player,
                legal_actions_O, legal_actions_X,
                (1,-1), action
            )
        return GameState(
            board, 1-self.current_player,
            legal_actions_O, legal_actions_X,
            (0,-1), action
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
    def __str__(self):
        out_list=[]
        for x in range(WID):
            out_list.append(chr(x+65)+" ")
        out_list.append("\n")
        for y in range(HEI):
            for x in range(WID):
                if(
                    (y,x)==self.last_action
                ):
                    out_list.append("@ ")
                    continue
                if(self.board[y*WID+x]==None):
                    legal_actions=(
                        self.legal_actions_X
                        if self.current_player
                        else self.legal_actions_O
                    )
                    if(
                        (y,x) in
                        legal_actions
                    ):
                        out_list.append("+ ")
                        continue
                    out_list.append(". ")
                    continue
                if(self.board[y*WID+x]==0):
                    out_list.append("O ")
                    continue
                if(self.board[y*WID+x]==1):
                    out_list.append("X ")
                    continue
            out_list.append(chr(y+1+ord("0"))+"\n")
        out=""
        out=out.join(out_list)
        return out
