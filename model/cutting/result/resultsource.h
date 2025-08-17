#pragma once

namespace Cutting{
namespace Result{


enum class ResultSource {
    FromStock,      // 🧱 Szálanyagból jött hulladék
    FromReusable,   // ♻️ Használt reusable darabból jött
    Unknown          // ❓ Ha nem egyértelmű
};

}}
