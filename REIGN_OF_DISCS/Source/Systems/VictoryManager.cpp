#include "Systems/VictoryManager.h"

bool VictoryManager::IsGameOver(const Board& board, const TurnManager& turn)
{
    return IsBoardFull(board) || IsBothPassed(turn);
}

GameResult VictoryManager::Judge(const Board& board)
{
    GameResult result;
    result.playerStones = board.CountStones(CellOwner::Player);
    result.enemyStones = board.CountStones(CellOwner::Enemy);
    if (result.playerStones > result.enemyStones)
    {
        result.winner = CellOwner::Player;
    }
    else if (result.enemyStones > result.playerStones)
    {
        result.winner = CellOwner::Enemy;
    }
    return result;
}
