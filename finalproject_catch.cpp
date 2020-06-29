#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <stdlib.h>
#include <time.h>
#include <chrono>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std;

class action{
public:
    int row, col, card, id, index;
    action()
    {
        row = -1;
        col = -1;
        card = -1;
        id = -1;
        index = -1;
    }
};

class Player {
public:
    vector<int> cards;
    string name;
    int id;
    void deleteCard(int index)
    {
        cards.erase(cards.begin()+index);
        return;
    }
    void printCards()
    {
        cout<<"["<<name<<" chess pieces]: [";
        for(int i=0; i<cards.size(); i++)
        {
            if(i==0) cout<<cards[i];
            else cout<<", "<<cards[i];
        }
        cout<<"]"<<endl;
    }
    Player(int mode, string n, int i)
    {
        this->name = n;
        this->id = i;
        if(mode == 4)
        {
            vector<int> tmp {2,3,5,8,13};
            cards = tmp;
        }
        else //mode == 6
        {
            vector<int> tmp {2,2,3,3,5,5,8,8,8,13,13};
            cards = tmp;
        }
    }
};

class Board{
public:
    vector<vector<int>> b;
    vector<vector<int>> whoscard;
    int size;
    
    void printBoard();
    int sumTable(int i, int j);
    void checkBoard();
    int checkWinner(Player p1, Player p2, int onOff);
    void doaction(action at);
    void randomMove(Player &pr);
    bool legalAction(action a);
    Board(int s){
        this->size = s;
        vector<int> row;
        for(int i=0; i<size; i++)
        {
            row.push_back(0);
        }
        for(int i=0; i<size; i++)
        {
            b.push_back(row);
            whoscard.push_back(row);
        }
    }
};

class node
{
public:
    int WinCount=0;
    int totalCount=0;
    node *children=nullptr;
    node *parent=nullptr;
    int childNum=0;
    float logTotalCount=0;
    bool terminal = false;
    action act;
    bool reallyExpand = false;
};

class MCTS
{
public:
    action search(Board, Player, Player);
    node* selection(node*, Board &, Player& pr1, Player& pr2);
    node * expansion(node *leaf, Board &bd, Player& pr1, Player& pr2);
    int simulation(node *newNode, Board &bd, Player& pr1, Player& pr2);
    void update(node *newNode, int winLose);
};

action MCTS::search(Board boardnow, Player pr1, Player pr2)
{
    node root;
    root.act.id = pr1.id;
    root.reallyExpand = true;
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    
    while(true)
    {
        //i++;
        Board btmp = boardnow;
        Player ptmp1 = pr1;
        Player ptmp2 = pr2;
        node *leaf = selection(&root, btmp, ptmp1, ptmp2);
        node *newNode = expansion(leaf, btmp, ptmp1, ptmp2);
        int winLose = simulation(newNode, btmp, ptmp1, ptmp2);
        update(newNode, winLose);
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() > 100)
        {
            break;
        }
    }
    action a;
    float score = -2;
    for(int i=0; i<root.childNum; i++)
    {
        const auto& child = root.children[i];
        float scoreChild = child.totalCount == 0 ? 0 : child.WinCount*1.0/child.totalCount + 1.414 * sqrt(root.logTotalCount/child.totalCount);
        if(score < scoreChild)
        {
            a = child.act;
            score = scoreChild;
        }
    }
    return a;
}

node* MCTS::selection(node* root, Board &tmp, Player& pr1, Player& pr2)
{
    node *ptr = root;
    while(ptr->childNum != 0)
    {
        int index=0;
        float score=-100;//-2
        for(int i=0; i<ptr->childNum; i++)
        {
            const auto& child = ptr->children[i];
            float scoreChild = child.totalCount == 0 ? 0 : child.WinCount*1.0/child.totalCount + 1.414 * sqrt(ptr->logTotalCount/child.totalCount);
            if(scoreChild - score > -0.0001)
            {
                if(scoreChild - score > 0.0001 )
                {
                    score = scoreChild;
                    index = i;
                }
                else
                {
                    //equal score;
                    if(rand()%2 == 0)
                    {
                        score = scoreChild;
                        index = i;
                    }
                }
            }
        }
        ptr = &ptr->children[index];
        tmp.doaction(ptr->act);
        if(ptr->act.id == pr1.id)
        {
            pr1.deleteCard(ptr->act.index);
        }
        else
        {
            pr2.deleteCard(ptr->act.index);
        }
    }
    return ptr;
}

node* MCTS::expansion(node *leaf, Board &bd, Player& pr1, Player& pr2)
{
    if(leaf->terminal) return leaf;
    if(leaf->reallyExpand == false) {
        leaf->reallyExpand = true;
        return leaf;
    }
    
    action actionarray[180];
    int previousCard = -1;
    int legalChildCount = 0;
    Player& pr = leaf->act.id == pr1.id ? pr2 : pr1;
    for(int i=0; i<pr.cards.size(); i++)
    {
        if(previousCard != pr.cards[i])
        {
            previousCard = pr.cards[i];
            int cardIndex = i;
            action tryThisAction;
            tryThisAction.card = pr.cards[i];
            tryThisAction.id = pr.id;
            tryThisAction.index = cardIndex;
            for(int j=0; j<bd.size; j++)
            {
                for(int k=0; k<bd.size; k++)
                {
                    tryThisAction.row = j;
                    tryThisAction.col = k;
                    if(bd.legalAction(tryThisAction))
                    {
                        actionarray[legalChildCount] = tryThisAction;
                        legalChildCount++;
                    }
                }
            }
        }
    }
    leaf->childNum = legalChildCount;
    leaf->terminal = legalChildCount == 0;
    if(leaf->terminal) return leaf;
    leaf->children = (node*) malloc(legalChildCount * sizeof(node));
    for(int i=0; i<legalChildCount; i++)
    {
        //initial
        leaf->children[i].WinCount = 0;
        leaf->children[i].totalCount = 0;
        leaf->children[i].children=nullptr;
        leaf->children[i].childNum=0;
        leaf->children[i].logTotalCount=0;
        leaf->children[i].terminal = false;
        leaf->children[i].reallyExpand = false;
        //assign value
        leaf->children[i].act = actionarray[i];
        leaf->children[i].parent = leaf;
    }
    int randomChoice = rand()%legalChildCount;
    bd.doaction(leaf->children[randomChoice].act);
    pr.deleteCard(leaf->children[randomChoice].act.index);
    leaf->children[randomChoice].reallyExpand = true;
    return &leaf->children[randomChoice];
}

int MCTS::simulation(node *newNode, Board &bd, Player &pr1, Player &pr2)
{
    int whoseTurn = -1;
    if(newNode->act.id == pr1.id)
    {
        whoseTurn = pr2.id;
    }
    else
    {
        whoseTurn = pr1.id;
    }
    while(true)
    {
        if(pr1.cards.size()==0 && pr2.cards.size()==0)
        {
            break;
        }
        if(whoseTurn == pr1.id)
        {
            whoseTurn = pr2.id;
            if(pr1.cards.size()==0) continue;
            bd.randomMove(pr1);
        }
        else
        {
            whoseTurn = pr1.id;
            if(pr2.cards.size()==0) continue;
            bd.randomMove(pr2);
        }
    }
    int whoWins = bd.checkWinner(pr1, pr2, 0);
    if(whoWins == newNode->act.id)
    {
        return 1; //win
    }
    else if(whoWins == -1)
    {
        return 0; //tie
    }
    else
    {
        return -1; //lose
    }
}

void MCTS::update(node *newNode,int winLose)
{
    while (newNode != nullptr) {
        newNode->totalCount++;
        newNode->WinCount += winLose;
        winLose = -winLose;
        newNode->logTotalCount = log(newNode->totalCount);
        newNode = newNode->parent;
    }
}

bool Board::legalAction(action a)
{
    int r = a.row, c = a.col;
    if(whoscard[r][c] == 0) return true;
    else return false;
}

void Board::printBoard()
{
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(whoscard[i][j]==-1) cout<<"X"<<" ";
            else cout<<b[i][j]<<" ";
        }
        cout<<endl;
    }
}

int Board::sumTable(int i, int j)
{
    int sum=0;
    for(int s=0; s<size; s++)
    {
        for(int t=0; t<size; t++)
        {
            if(abs(s-i)<=1 && abs(t-j)<=1)
            {
                sum += b[s][t];
            }
        }
    }
    return sum;
}

void Board::checkBoard()
{
    vector<vector<int>> mark;
    vector<int> row;
    for(int i=0; i<size; i++)
    {
        row.push_back(0);
    }
    for(int i=0; i<size; i++)
    {
        mark.push_back(row);
    }
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(b[i][j]!=0 && whoscard[i][j]!=-1 && sumTable(i, j) > 15)
            {
                mark[i][j] = 1;
            }
        }
    }
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(mark[i][j]==1)
            {
                whoscard[i][j] = -1;
                b[i][j] = 0;
            }
        }
    }
}

int Board::checkWinner(Player p1, Player p2, int onOff)
{
    //num of 2, 3, 5, 8, 13 on the board
    int m1[5] = {0, 0, 0, 0, 0};
    int m2[5] = {0, 0, 0, 0, 0};
    int p1Score = 0;
    int p2Score = 0;
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(whoscard[i][j] == p1.id)
            {
                p1Score += b[i][j];
                if(b[i][j] == 2)
                {
                    m1[0]++;
                }
                else if(b[i][j] == 3)
                {
                    m1[1]++;
                }
                else if(b[i][j] == 5)
                {
                    m1[2]++;
                }
                else if(b[i][j] == 8)
                {
                    m1[3]++;
                }
                else if(b[i][j] == 13)
                {
                    m1[4]++;
                }
            }
            else if(whoscard[i][j] == p2.id)
            {
                p2Score += b[i][j];
                if(b[i][j] == 2)
                {
                    m2[0]++;
                }
                else if(b[i][j] == 3)
                {
                    m2[1]++;
                }
                else if(b[i][j] == 5)
                {
                    m2[2]++;
                }
                else if(b[i][j] == 8)
                {
                    m2[3]++;
                }
                else if(b[i][j] == 13)
                {
                    m2[4]++;
                }
            }
        }
    }
    if(p1Score > p2Score)
    {
        if(onOff==1) cout<<p1.name<<" Wins."<<endl;
        return p1.id;
    }
    else if(p1Score < p2Score)
    {
        if(onOff==1) cout<<p2.name<<" Wins."<<endl;
        return p2.id;
    }
    for(int i=4; i>=0; i--)
    {
        if(m1[i] > m2[i])
        {
            if(onOff==1) cout<<p1.name<<" Wins."<<endl;
            return p1.id;
        }
        else if(m1[i] < m2[i])
        {
             if(onOff==1) cout<<p2.name<<" Wins."<<endl;
            return p2.id;
        }
    }
     if(onOff==1) cout<<"Tie."<<endl;
    return -1;
}

void Board::doaction(action at)
{
    b[at.row][at.col] = at.card;
    whoscard[at.row][at.col] = at.id;
    checkBoard();
}

void Board::randomMove(Player &pr)
{
    int availablePlace = 0;
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(whoscard[i][j] == 0)
            {
                availablePlace++;
            }
        }
    }
    int randPosition = (rand() % availablePlace) + 1;
    action a;
    int flag=0;
    for(int i=0; i<size; i++)
    {
        for(int j=0; j<size; j++)
        {
            if(whoscard[i][j] == 0)
            {
                randPosition--;
                if(randPosition == 0)
                {
                    flag = 1;
                    a.row = i;
                    a.col = j;
                    break;
                }
            }
        }
        if(flag==1) break;
    }
    int randCardIndex = rand() % pr.cards.size();
    int randCard = pr.cards[randCardIndex];
    a.card = randCard;
    a.index = randCardIndex;
    a.id = pr.id;
    doaction(a);
    pr.deleteCard(a.index);
    return;
}

action query(Board bd, Player pr)
{
    action a;
    while(a.card == -1)
    {
        int row, col, weight;
        cout<<"Input(row, col, weight): ";
        cin >> row >> col >> weight;
        //boundary check
        if(row >= bd.size || col >= bd.size)
        {
            cout<<"Out of boundary! Please choose another position."<<endl;
            continue;
        }
        //card check
        int exist=0, index = -1;
        for(int i=0; i<pr.cards.size(); i++)
        {
            if(pr.cards[i] == weight)
            {
                exist = 1;
                index = i;
            }
        }
        if(exist == 0)
        {
            cout<<"You don't have this card: "<<weight<<endl;
            cout<<"Please choose another card."<<endl;
            continue;
        }
        //occupied check
        if(bd.whoscard[row][col] != 0)
        {
            cout<<"Position can not be reused. Please choose another position"<<endl;
            continue;
        }
        a.card = weight;
        a.row = row;
        a.col = col;
        a.id = pr.id;
        a.index = index;
    }
    return a;
}

void printAction(Player p, action a)
{
    cout<<"["<<p.name<<"] : ("<<a.row<<", "<<a.col<<", "<<a.card<<")"<<endl;
}

TEST_CASE("action/ constructor", "[action]"){
    action a;
    REQUIRE(a.row == -1);
    REQUIRE(a.col == -1);
    REQUIRE(a.card == -1);
    REQUIRE(a.id == -1);
    REQUIRE(a.index == -1);
}

TEST_CASE("Player/ deleteCard", "[Player]"){
    Player p1(4, "user", 0);
    p1.deleteCard(0);
    REQUIRE(p1.cards[0] == 3);
    REQUIRE(p1.cards[1] == 5);
    REQUIRE(p1.cards[2] == 8);
    REQUIRE(p1.cards[3] == 13);
}

TEST_CASE("Player/ printCards", "[Player]"){
    Player p1(4, "user", 0);
    std::stringstream buffer;
    std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
    p1.printCards();
    std::string text = buffer.str();
    std::cout.rdbuf(old);
    REQUIRE(text == "[user chess pieces]: [2, 3, 5, 8, 13]\n");
}

TEST_CASE("Player/ constructor_1", "[Player]"){
    Player p1(4, "user", 0);
    vector<int> tmp {2,3,5,8,13};
    bool a = p1.cards==tmp;
    REQUIRE(a == true);
}

TEST_CASE("Player/ constructor_2", "[Player]"){
    Player p1(6, "user", 0);
    vector<int> tmp {2,2,3,3,5,5,8,8,8,13,13};
    bool a = p1.cards==tmp;
    REQUIRE(a == true);
}

TEST_CASE("Board/ printBoard_1", "[Board]"){
    Board b1(4);
    std::stringstream buffer;
    std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
    b1.printBoard();
    std::string text = buffer.str();
    std::cout.rdbuf(old);
    REQUIRE(text == "0 0 0 0 \n0 0 0 0 \n0 0 0 0 \n0 0 0 0 \n");
}

TEST_CASE("Board/ printBoard_2", "[Board]"){
    Board b1(4);
    b1.whoscard[0][0] = -1;
    std::stringstream buffer;
    std::streambuf * old = std::cout.rdbuf(buffer.rdbuf());
    b1.printBoard();
    std::string text = buffer.str();
    std::cout.rdbuf(old);
    REQUIRE(text == "X 0 0 0 \n0 0 0 0 \n0 0 0 0 \n0 0 0 0 \n");
}

TEST_CASE("Board/ sumTable", "[Board]"){
    Board b1(4);
    b1.b[0][0] = 13;
    b1.b[0][2] = 2;
    REQUIRE(b1.sumTable(0, 0) == 13);
}

TEST_CASE("Board/ checkBoard", "[Board]"){
    Board b1(4);
    b1.b[0][0] = 13;
    b1.b[0][1] = 3;
    b1.b[0][2] = 2;
    b1.whoscard[0][0] = 1;
    b1.whoscard[0][1] = 1;
    b1.whoscard[0][2] = 1;
    b1.checkBoard();
    REQUIRE(b1.b[0][0] == 0);
    REQUIRE(b1.whoscard[0][0] == -1);
    REQUIRE(b1.b[0][1] == 0);
    REQUIRE(b1.whoscard[0][1] == -1);
    REQUIRE(b1.b[0][2] == 2);
    REQUIRE(b1.whoscard[0][2] == 1);
}

TEST_CASE("Board/ checkWinner_1", "[Board]"){
    Board b1(6);
    Player p0(6,"user1", 0);
    Player p1(6,"user2", 1);
    b1.b[0][0] = 2;
    b1.b[0][1] = 3;
    b1.b[0][2] = 5;
    b1.b[0][3] = 8;
    b1.b[1][0] = 13;
    b1.b[1][1] = 2;
    b1.whoscard[0][0] = p0.id;
    b1.whoscard[0][1] = p0.id;
    b1.whoscard[0][2] = p0.id;
    b1.whoscard[0][3] = p0.id;
    b1.whoscard[1][0] = p0.id;
    b1.whoscard[1][1] = p0.id;
    b1.b[2][0] = 2;
    b1.b[2][1] = 3;
    b1.b[2][2] = 5;
    b1.b[2][3] = 8;
    b1.b[3][1] = 13;
    b1.whoscard[2][0] = p1.id;
    b1.whoscard[2][1] = p1.id;
    b1.whoscard[2][2] = p1.id;
    b1.whoscard[2][3] = p1.id;
    b1.whoscard[3][1] = p1.id;
    REQUIRE(b1.checkWinner(p0,p1,0) == p0.id);
}

TEST_CASE("Board/ checkWinner_2", "[Board]"){
    Board b1(6);
    Player p0(6,"user1", 0);
    Player p1(6,"user2", 1);
    b1.b[0][0] = 2;
    b1.b[0][1] = 3;
    b1.b[0][2] = 5;
    b1.b[0][3] = 8;
    b1.b[1][0] = 13;
    b1.whoscard[0][0] = p0.id;
    b1.whoscard[0][1] = p0.id;
    b1.whoscard[0][2] = p0.id;
    b1.whoscard[0][3] = p0.id;
    b1.whoscard[1][0] = p0.id;
    b1.b[2][0] = 2;
    b1.b[2][1] = 3;
    b1.b[2][2] = 5;
    b1.b[2][3] = 8;
    b1.b[3][1] = 13;
    b1.b[3][2] = 2;
    b1.whoscard[2][0] = p1.id;
    b1.whoscard[2][1] = p1.id;
    b1.whoscard[2][2] = p1.id;
    b1.whoscard[2][3] = p1.id;
    b1.whoscard[3][1] = p1.id;
    b1.whoscard[3][2] = p1.id;
    REQUIRE(b1.checkWinner(p0,p1,0) == p1.id);
}

TEST_CASE("Board/ checkWinner_3", "[Board]"){
    Board b1(6);
    Player p0(6,"user1", 0);
    Player p1(6,"user2", 1);
    b1.b[0][0] = 2;
    b1.b[0][1] = 3;
    b1.b[0][2] = 5;
    b1.b[1][0] = 13;
    b1.whoscard[0][0] = p0.id;
    b1.whoscard[0][1] = p0.id;
    b1.whoscard[0][2] = p0.id;
    b1.whoscard[1][0] = p0.id;
    b1.b[2][0] = 2;
    b1.b[2][3] = 8;
    b1.b[3][1] = 13;
    b1.whoscard[2][0] = p1.id;
    b1.whoscard[2][3] = p1.id;
    b1.whoscard[3][1] = p1.id;
    REQUIRE(b1.checkWinner(p0,p1,0) == p1.id);
}

TEST_CASE("Board/ checkWinner_4", "[Board]"){
    Board b1(6);
    Player p0(6,"user1", 0);
    Player p1(6,"user2", 1);
    b1.b[0][0] = 2;
    b1.b[0][1] = 8;
    b1.b[1][0] = 13;
    b1.whoscard[0][0] = p0.id;
    b1.whoscard[0][1] = p0.id;
    b1.whoscard[1][0] = p0.id;
    b1.b[2][0] = 2;
    b1.b[2][1] = 3;
    b1.b[2][3] = 5;
    b1.b[3][1] = 13;
    b1.whoscard[2][0] = p1.id;
    b1.whoscard[2][1] = p1.id;
    b1.whoscard[2][3] = p1.id;
    b1.whoscard[3][1] = p1.id;
    REQUIRE(b1.checkWinner(p0,p1,0) == p0.id);
}

TEST_CASE("Board/ checkWinner_5", "[Board]"){
    Board b1(6);
    Player p0(6,"user1", 0);
    Player p1(6,"user2", 1);
    b1.b[0][0] = 2;
    b1.b[0][1] = 8;
    b1.b[1][0] = 13;
    b1.whoscard[0][0] = p0.id;
    b1.whoscard[0][1] = p0.id;
    b1.whoscard[1][0] = p0.id;
    b1.b[2][0] = 2;
    b1.b[2][3] = 8;
    b1.b[3][1] = 13;
    b1.whoscard[2][0] = p1.id;
    b1.whoscard[2][3] = p1.id;
    b1.whoscard[3][1] = p1.id;
    REQUIRE(b1.checkWinner(p0,p1,0) == -1);
}

TEST_CASE("Board/ doaction", "[Board]"){
    Board b1(4);
    action a;
    a.row = 1;
    a.col = 3;
    a.card = 2;
    a.id = 0;
    a.index = 0;
    b1.doaction(a);
    REQUIRE(b1.b[1][3] == 2);
    REQUIRE(b1.whoscard[1][3] == 0);
}

TEST_CASE("Board/ randomMove", "[Board]"){
    Board b1(4);
    Player p1(4, "user", 1);
    b1.randomMove(p1);
    int row_1 = -1, col_1 = -1, row_2 = -1, col_2 = -1;;
    for(int i=0; i<b1.size; i++)
    {
        for(int j=0; j<b1.size; j++)
        {
            if(b1.b[i][j]!=0)
            {
                row_1 = i;
                col_1 = j;
            }
        }
    }
    for(int i=0; i<b1.size; i++)
    {
        for(int j=0; j<b1.size; j++)
        {
            if(b1.whoscard[i][j]!=0)
            {
                row_2 = i;
                col_2 = j;
            }
        }
    }
    REQUIRE(row_1 == row_2);
    REQUIRE(col_1 == col_2);
    REQUIRE(b1.whoscard[row_1][col_1] == p1.id);
}

TEST_CASE("Board/ legalAction_1", "[Board]"){
    Board b1(4);
    action a;
    a.row = 1;
    a.col = 3;
    a.card = 2;
    a.id = 0;
    a.index = 0;
    REQUIRE(b1.legalAction(a) == true);
}

TEST_CASE("Board/ legalAction_2", "[Board]"){
    Board b1(4);
    b1.whoscard[1][3] = 2;
    action a;
    a.row = 1;
    a.col = 3;
    a.card = 2;
    a.id = 0;
    a.index = 0;
    REQUIRE(b1.legalAction(a) == false);
}

TEST_CASE("Board/ constructor", "[Board]"){
    Board b1(4);
    REQUIRE(b1.b.size() == 4);
    REQUIRE(b1.b[0].size() == 4);
    REQUIRE(b1.whoscard.size() == 4);
    REQUIRE(b1.whoscard[0].size() == 4);
}

TEST_CASE("MCTS/ search", "[MCTS]"){
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    MCTS m;
    action a = m.search(b1,p1,p2);
    REQUIRE(a.row != -1);
    REQUIRE(a.col != -1);
    REQUIRE(a.id != -1);
    REQUIRE(a.index != -1);
    REQUIRE(a.card != -1);
    REQUIRE(b1.b[a.row][a.col] == 0);
    REQUIRE(b1.whoscard[a.row][a.col] == 0);
}

TEST_CASE("MCTS/ selection_1", "[MCTS]"){
    MCTS m;
    node root;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    node *ptr = m.selection(&root, b1, p1, p2);
    REQUIRE(&root == ptr);
}

TEST_CASE("MCTS/ selection_2", "[MCTS]"){
    MCTS m;
    node root;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root.children = (node*) malloc(1 * sizeof(node));
    action a;
    a.row = 0;
    a.col = 0;
    a.card = 2;
    a.id = 1;
    a.index = 0;

    root.children[0].WinCount = 0;
    root.children[0].totalCount = 0;
    root.children[0].children=nullptr;
    root.children[0].childNum=0;
    root.children[0].logTotalCount=0;
    root.children[0].terminal = false;
    root.children[0].reallyExpand = false;
    root.children[0].act = a;
    root.children[0].parent = &root;
    root.childNum = 1;

    node *ptr = m.selection(&root, b1, p1, p2);
    REQUIRE(&root.children[0] == ptr);
}

TEST_CASE("MCTS/ expansion_1", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->terminal = true;
    node *tmp = m.expansion(root, b1,p1,p2);
    REQUIRE(tmp == root);
}

TEST_CASE("MCTS/ expansion_2", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->reallyExpand = false;
    node *tmp = m.expansion(root, b1,p1,p2);
    REQUIRE(tmp == root);
}

TEST_CASE("MCTS/ expansion_3", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->reallyExpand = true;
    root->terminal = false;
    root->act.id = p1.id;
    p2.deleteCard(0);
    p2.deleteCard(0);
    p2.deleteCard(0);
    p2.deleteCard(0);
    p2.deleteCard(0);
    node *tmp = m.expansion(root, b1,p1,p2);
    REQUIRE(tmp == root);
}

TEST_CASE("MCTS/ expansion_4", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->reallyExpand = true;
    root->terminal = false;
    root->act.id = p1.id;
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<4; j++)
        {
            b1.whoscard[i][j] = 1;
        }
    }
    node *tmp = m.expansion(root, b1,p1,p2);
    REQUIRE(tmp == root);
}

TEST_CASE("MCTS/ expansion_5", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->reallyExpand = true;
    root->terminal = false;
    root->act.id = p1.id;
    node *tmp = m.expansion(root, b1,p1,p2);
    REQUIRE(tmp->act.row != -1);
    REQUIRE(tmp->act.col != -1);
    REQUIRE(tmp->act.id != -1);
    REQUIRE(tmp->act.index != -1);
    REQUIRE(tmp->act.card != -1);
}

TEST_CASE("MCTS/ simulation_1", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->act.id = p1.id;
    for(int i=0; i<5; i++)
    {
        p1.deleteCard(0);
        p2.deleteCard(0);
    }
    REQUIRE(m.simulation(root, b1, p1, p2) == 0);
}

TEST_CASE("MCTS/ simulation_2", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->act.id = p2.id;
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);

    p2.deleteCard(1);
    p2.deleteCard(1);
    p2.deleteCard(1);
    p2.deleteCard(1);
    REQUIRE(m.simulation(root, b1, p1, p2) == -1);
}

TEST_CASE("MCTS/ simulation_3", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    root->act.id = p1.id;
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);

    p2.deleteCard(1);
    p2.deleteCard(1);
    p2.deleteCard(1);
    p2.deleteCard(1);
    REQUIRE(m.simulation(root, b1, p1, p2) == 1);
}

TEST_CASE("MCTS/ update_1", "[MCTS]"){
    MCTS m;
    node *root = (node*) malloc(1 * sizeof(node));
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    action a;
    a.row = 0;
    a.col = 0;
    a.card = 2;
    a.id = 1;
    a.index = 0;
    root->WinCount = 0;
    root->totalCount = 0;
    root->children=nullptr;
    root->childNum=0;
    root->logTotalCount=0;
    root->terminal = false;
    root->reallyExpand = false;
    root->act = a;
    root->parent = nullptr;
    m.update(root, 1);
    REQUIRE(root->totalCount == 1);
    REQUIRE(root->WinCount == 1);
    REQUIRE(root->logTotalCount == log(1));
    REQUIRE(root->parent == nullptr);
}

TEST_CASE("MCTS/ update_2", "[MCTS]"){
    MCTS m;
    node *root = nullptr;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    m.update(root, 1);
    REQUIRE(root == nullptr);
}

TEST_CASE("util_func/ query_1", "[util_func]"){
    MCTS m;
    node *root = nullptr;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    std::stringstream cout_buffer;
    std::stringstream cin_buffer;
    cin_buffer<<"4 4 3 0 0 3";
    std::streambuf * old_out = std::cout.rdbuf(cout_buffer.rdbuf());
    std::streambuf * old_in = std::cin.rdbuf(cin_buffer.rdbuf());
    action a = query(b1,p1);
    std::string out_text = cout_buffer.str();
    std::cout.rdbuf(old_out);
    std::cin.rdbuf(old_in);
    REQUIRE(out_text == "Input(row, col, weight): Out of boundary! Please choose another position.\nInput(row, col, weight): ");
    REQUIRE(a.row == 0);
    REQUIRE(a.col == 0);
    REQUIRE(a.id == p1.id);
    REQUIRE(a.index == 1);
    REQUIRE(a.card == 3);
}

TEST_CASE("util_func/ query_2", "[util_func]"){
    MCTS m;
    node *root = nullptr;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);
    p1.deleteCard(0);
    std::stringstream cout_buffer;
    std::stringstream cin_buffer;
    cin_buffer<<"0 0 3 0 0 13";
    std::streambuf * old_out = std::cout.rdbuf(cout_buffer.rdbuf());
    std::streambuf * old_in = std::cin.rdbuf(cin_buffer.rdbuf());
    action a = query(b1,p1);
    std::string out_text = cout_buffer.str();
    std::cout.rdbuf(old_out);
    std::cin.rdbuf(old_in);
    REQUIRE(out_text == "Input(row, col, weight): You don't have this card: 3\nPlease choose another card.\nInput(row, col, weight): ");
    REQUIRE(a.row == 0);
    REQUIRE(a.col == 0);
    REQUIRE(a.id == p1.id);
    REQUIRE(a.index == 0);
    REQUIRE(a.card == 13);
}

TEST_CASE("util_func/ query_3", "[util_func]"){
    MCTS m;
    node *root = nullptr;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    b1.whoscard[0][0] = 1;
    std::stringstream cout_buffer;
    std::stringstream cin_buffer;
    cin_buffer<<"0 0 3 1 1 13";
    std::streambuf * old_out = std::cout.rdbuf(cout_buffer.rdbuf());
    std::streambuf * old_in = std::cin.rdbuf(cin_buffer.rdbuf());
    action a = query(b1,p1);
    std::string out_text = cout_buffer.str();
    std::cout.rdbuf(old_out);
    std::cin.rdbuf(old_in);
    REQUIRE(out_text == "Input(row, col, weight): Position can not be reused. Please choose another position\nInput(row, col, weight): ");
    REQUIRE(a.row == 1);
    REQUIRE(a.col == 1);
    REQUIRE(a.id == p1.id);
    REQUIRE(a.index == 4);
    REQUIRE(a.card == 13);
}

TEST_CASE("util_func/ printAction", "[util_func]"){
    MCTS m;
    node *root = nullptr;
    Board b1(4);
    Player p1(4, "user1", 1);
    Player p2(4, "user2", 2);
    action a;
    a.row = 0;
    a.col = 1;
    a.card = 2;
    a.index = 0;
    a.id = p1.id;
    std::stringstream cout_buffer;
    std::streambuf * old_out = std::cout.rdbuf(cout_buffer.rdbuf());
    printAction(p1,a);
    std::string out_text = cout_buffer.str();
    std::cout.rdbuf(old_out);
    REQUIRE(out_text == "[user1] : (0, 1, 2)\n");
}