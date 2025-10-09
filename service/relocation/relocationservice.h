// service/relocation/relocationservice.h
#pragma once
#include "model/relocation/relocationinstruction.h"
#include "presenter/CuttingPresenter.h"

class RelocationService {
public:
    static bool finalize(RelocationInstruction& instr, CuttingPresenter* presenter);
    //static RelocationInstruction finalize(const RelocationInstruction& instr);
    static void rollback(const RelocationInstruction& instr); // később Sprint 5
};
