#include "stockrepository.h"
#include <ranges>

StockRepository& StockRepository::instance() {
    static StockRepository repo;
    return repo;
}

void StockRepository::add(const StockEntry& entry) {
    _stock.append(entry);
}

void StockRepository::clear() {
    _stock.clear();
}

QVector<StockEntry> StockRepository::all() const {
    return _stock;
}

bool StockRepository::removeByMaterialId(const QUuid& id) {
    const int index = std::ranges::find_if(_stock, [&](const auto& s) { return s.materialId == id; }) - _stock.begin();
    if (index >= 0 && index < _stock.size()) {
        _stock.remove(index);
        return true;
    }
    return false;
}

QVector<StockEntry> StockRepository::findByCategory(ProfileCategory cat) const {
    QVector<StockEntry> result;
    for (const auto& s : _stock)
        if (s.category() == cat)
            result.append(s);
    return result;
}
