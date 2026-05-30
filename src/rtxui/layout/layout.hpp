#ifndef RTXUI_LAYOUT_LAYOUT_HPP
#define RTXUI_LAYOUT_LAYOUT_HPP

#include <memory>

#include "rtxui/layout/layout_box.hpp"
#include "rtxui/layout/physical_fragment.hpp"
#include "rtxui/layout/style.hpp"

namespace rtxui {
std::shared_ptr<PhysicalFragment> RunLayout(LayoutInputNode node,
                                            LayoutConstraints constraints);
}  // namespace rtxui

#endif  // RTXUI_LAYOUT_LAYOUT_HPP
