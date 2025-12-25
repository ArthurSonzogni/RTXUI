#ifndef RTXUI_LAYOUT_LAYOUT_HPP
#define RTXUI_LAYOUT_LAYOUT_HPP

#include <memory>
#include "layout/layout_box.hpp"
#include "layout/physical_fragment.hpp"
#include "layout/style.hpp"

namespace rtxui {
std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints);
}  // namespace rtxui

#endif  // RTXUI_LAYOUT_LAYOUT_HPP
