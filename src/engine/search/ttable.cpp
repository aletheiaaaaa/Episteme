#include "ttable.h"

namespace episteme::tt {
    Table::Table(uint32_t size) {
        const size_t entries = (size * 1024 * 1024) / sizeof(Entry);
        ttable.resize(entries);
    }
}