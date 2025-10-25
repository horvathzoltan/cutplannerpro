#pragma once

#include "model/cutting/optimizer/optimizermodel.h"

struct OptimizationRunner {
    static void run(Cutting::Optimizer::OptimizerModel& model,
                    Cutting::Optimizer::TargetHeuristic heuristic) {
        // 🚀 Optimalizáció futtatása
        model.optimize(heuristic);

        // TODO: ha kell, itt lehet gépId hozzárendelés, kerf beállítás
    }
};

