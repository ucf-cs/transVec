#ifndef STOVEC_HPP
#define STOVEC_HPP

#include "transaction.hpp"
#include "TVector.hh"
#include "threadLocalGlobals.hpp"

class STOVector
{
private:
    TVector<VAL> vector;

public:
    void executeTransaction(Desc *desc)
    {
        hasAborted = false;
        { // Scoped section.
            TransactionGuard t;
            for (size_t i = 0; i < desc->size; i++)
            {
                Operation *op = &desc->ops[i];
                switch (op->type)
                {
                case Operation::OpType::read:
                    op->ret = vector[op->index];
                    break;
                case Operation::OpType::write:
                    vector[op->index] = op->val;
                    break;
                case Operation::OpType::pushBack:
                    vector.push_back(op->val);
                    break;
                case Operation::OpType::popBack:
                    op->ret = vector[vector.size() - 1];
                    vector.pop_back();
                    break;
                case Operation::OpType::size:
                    op->ret = vector.size();
                    break;
                case Operation::OpType::reserve:
                    // TODO: There is not a transactionally-safe reserve operation. (Only nontrans_reserve())
                    //vector.nontrans_reserve(op->index);
                    // Do nothing for now.
                    break;
                default:
                    hasAborted = true;
                    break;
                }

                if (hasAborted)
                {
                    break;
                }
            }
        }
        if (!hasAborted)
        {
            desc->status.store(Desc::TxStatus::committed);
        }
        else
        {
            desc->status.store(Desc::TxStatus::aborted);
        }
        return;
    }
    void printContents()
    {
        // NOTE: Stubbed function for now.
        // Implement it if we want it somewhere down the line.
        return;
    }
};

#endif