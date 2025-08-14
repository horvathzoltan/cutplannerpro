#pragma once

namespace Cutting{
namespace Result{


enum class Source {
    FromStock,      // 🧱 Szálanyagból jött hulladék
    FromReusable,   // ♻️ Használt reusable darabból jött
    Unknown          // ❓ Ha nem egyértelmű
};

}}
