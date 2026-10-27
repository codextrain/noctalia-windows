// Rust · layout.rs · geometry engine · Windows 11 · stable Rust
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Rect {
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}
impl Rect {
    pub fn width(self) -> i32 {
        self.right - self.left
    }
    pub fn height(self) -> i32 {
        self.bottom - self.top
    }
}
/// A finite viewport over an ordered strip. Nonvisible columns are minimized,
/// never placed on another monitor. The focused column is always fully visible.
#[cfg(test)]
pub fn layout(
    area: Rect,
    count: usize,
    focus: usize,
    gap: i32,
    percent: i32,
    top: i32,
) -> Vec<Option<Rect>> {
    group_layout(
        area,
        &(0..count).map(|i| i / 2).collect::<Vec<_>>(),
        focus,
        gap,
        percent,
        top,
    )
}

/// Stable workspace memberships: one centered window or two equal columns.
pub fn group_layout(
    area: Rect,
    groups: &[usize],
    focus: usize,
    gap: i32,
    percent: i32,
    top: i32,
) -> Vec<Option<Rect>> {
    let count = groups.len();
    if count == 0 || area.width() < 160 + gap * 2 || area.height() < 100 + gap * 2 + top {
        return vec![None; count];
    }
    let selected = groups[focus.min(count - 1)];
    let members: Vec<usize> = groups
        .iter()
        .enumerate()
        .filter(|(_, g)| **g == selected)
        .map(|(i, _)| i)
        .collect();
    let width = if members.len() == 1 {
        ((area.width() - gap * 2) * percent / 100)
            .max(160)
            .min(area.width() - gap * 2)
    } else {
        (area.width() - gap * 3) / 2
    };
    if width < 160 {
        return vec![None; count];
    }
    (0..count)
        .map(|i| {
            if let Some(slot) = members.iter().position(|member| *member == i) {
                let x = if members.len() == 1 {
                    area.left + (area.width() - width) / 2
                } else {
                    area.left + gap + slot as i32 * (width + gap)
                };
                Some(Rect {
                    left: x,
                    top: area.top + gap + top,
                    right: x + width,
                    bottom: area.bottom - gap,
                })
            } else {
                None
            }
        })
        .collect()
}

pub fn new_group(groups: &[usize]) -> usize {
    let Some(last) = groups.iter().copied().max() else {
        return 0;
    };
    if groups.iter().filter(|g| **g == last).count() < 2 {
        last
    } else {
        last + 1
    }
}
pub fn drop_group(groups: &[usize], active: Option<usize>) -> usize {
    let Some(last) = groups.iter().copied().max() else {
        return 0;
    };
    let group = active.filter(|g| groups.contains(g)).unwrap_or(last);
    if groups.iter().filter(|g| **g == group).count() < 2 {
        group
    } else {
        last + 1
    }
}
pub fn spotlight_layout(
    area: Rect,
    groups: &[usize],
    focus: usize,
    gap: i32,
    percent: i32,
    top: i32,
    spotlight: Option<usize>,
) -> Vec<Option<Rect>> {
    if let Some(index) =
        spotlight.filter(|i| *i < groups.len() && groups[*i] == groups[focus.min(groups.len() - 1)])
    {
        let mut result = vec![None; groups.len()];
        result[index] = group_layout(area, &[groups[index]], 0, gap, percent, top)[0];
        result
    } else {
        group_layout(area, groups, focus, gap, percent, top)
    }
}

pub fn neighbor_group(groups: &[usize], focus: usize, right: bool) -> usize {
    let current = groups[focus.min(groups.len() - 1)];
    let target = if right {
        groups.iter().copied().filter(|g| *g > current).min()
    } else {
        groups.iter().copied().filter(|g| *g < current).max()
    }
    .unwrap_or(current);
    if target == current {
        focus
    } else {
        groups.iter().position(|g| *g == target).unwrap_or(focus)
    }
}
pub fn covers(window: Rect, monitor: Rect) -> bool {
    window.left <= monitor.left + 2
        && window.top <= monitor.top + 2
        && window.right >= monitor.right - 2
        && window.bottom >= monitor.bottom - 2
}
pub fn bounded_top_bar(bounds: Rect, bar: Rect) -> Option<Rect> {
    (bar.width() >= bounds.width() / 2
        && bar.height() > 0
        && bar.height() <= 160
        && bar.top >= bounds.top
        && bar.top <= bounds.top + 80
        && bar.left >= bounds.left - 32
        && bar.right <= bounds.right + 32)
        .then_some(Rect {
            left: bar.left.max(bounds.left),
            right: bar.right.min(bounds.right),
            ..bar
        })
}
/// Convert a visible-frame target to Win32's outer rectangle, including invisible borders.
pub fn outer_for_visible(target: Rect, outer: Rect, frame: Rect) -> Rect {
    let margins = [
        frame.left - outer.left,
        frame.top - outer.top,
        outer.right - frame.right,
        outer.bottom - frame.bottom,
    ];
    if margins.iter().any(|margin| !(0..=32).contains(margin)) {
        return target;
    }
    Rect {
        left: target.left - margins[0],
        top: target.top - margins[1],
        right: target.right + margins[2],
        bottom: target.bottom + margins[3],
    }
}
pub fn interpolate(start: Rect, end: Rect, progress: f64) -> Rect {
    let t = progress.clamp(0.0, 1.0);
    let ease = t * t * (3.0 - 2.0 * t);
    let mix = |a: i32, b: i32| (f64::from(a) + f64::from(b - a) * ease).round() as i32;
    Rect {
        left: mix(start.left, end.left),
        top: mix(start.top, end.top),
        right: mix(start.right, end.right),
        bottom: mix(start.bottom, end.bottom),
    }
}
/// Outer layout margins are compensated so column edges follow the bar.
/// The bar's bottom is combined with the existing work area, never added twice.
pub fn bar_area(work: Rect, bar: Rect, gap: i32) -> Rect {
    Rect {
        left: (bar.left - gap).max(work.left),
        right: (bar.right + gap).min(work.right),
        top: work.top.max(bar.bottom),
        bottom: work.bottom,
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn drop_joins_the_active_workspace_only_when_it_has_space() {
        assert_eq!(drop_group(&[], None), 0);
        assert_eq!(drop_group(&[0], Some(0)), 0);
        assert_eq!(drop_group(&[0, 0], Some(0)), 1);
        assert_eq!(drop_group(&[0, 0, 1], Some(0)), 2);
        assert_eq!(drop_group(&[0, 0, 1], Some(1)), 1);
    }
    #[test]
    fn spotlight_centers_selected_member_without_changing_workspace_membership() {
        let area = Rect {
            left: 0,
            top: 36,
            right: 1920,
            bottom: 1032,
        };
        let groups = [0, 0, 1, 1];
        let shown = spotlight_layout(area, &groups, 1, 12, 55, 0, Some(1));
        assert!(shown[0].is_none() && shown[2].is_none() && shown[3].is_none());
        let r = shown[1].unwrap();
        assert!((r.left - (1920 - r.right)).abs() <= 1);
        let restored = spotlight_layout(area, &groups, 1, 12, 55, 0, None);
        assert_eq!(restored.iter().flatten().count(), 2);
        assert_eq!(groups, [0, 0, 1, 1]);
        assert_eq!(
            spotlight_layout(area, &groups, 2, 12, 55, 0, Some(1)),
            group_layout(area, &groups, 2, 12, 55, 0)
        );
    }
    #[test]
    fn full_width_bar_shadow_padding_does_not_hide_reserved_top_space() {
        let monitor = Rect {
            left: 0,
            top: 0,
            right: 1920,
            bottom: 1080,
        };
        assert_eq!(
            bounded_top_bar(
                monitor,
                Rect {
                    left: -14,
                    top: 0,
                    right: 1934,
                    bottom: 36
                }
            ),
            Some(Rect {
                left: 0,
                top: 0,
                right: 1920,
                bottom: 36
            })
        );
        assert_eq!(
            bounded_top_bar(
                monitor,
                Rect {
                    left: -100,
                    top: 0,
                    right: 2020,
                    bottom: 36
                }
            ),
            None
        );
    }
    #[test]
    fn animation_eases_without_overshoot_and_finishes_at_exact_rectangle() {
        let a = Rect {
            left: -1920,
            top: 100,
            right: -1200,
            bottom: 700,
        };
        let b = Rect {
            left: 100,
            top: 48,
            right: 1908,
            bottom: 1020,
        };
        assert_eq!(interpolate(a, b, 0.0), a);
        assert_eq!(interpolate(a, b, 1.0), b);
        assert_eq!(interpolate(a, b, 2.0), b);
        let frames: Vec<_> = (0..=10)
            .map(|i| interpolate(a, b, f64::from(i) / 10.0))
            .collect();
        assert!(frames.windows(2).all(|pair| pair[0].left <= pair[1].left));
        assert!(frames
            .iter()
            .all(|r| r.left >= a.left && r.left <= b.left && r.width() > 0 && r.height() > 0));
    }
    #[test]
    fn visible_frames_align_despite_different_invisible_borders() {
        let target = Rect {
            left: 12,
            top: 48,
            right: 954,
            bottom: 1020,
        };
        let chrome_frame = Rect {
            left: 19,
            top: 48,
            right: 947,
            bottom: 1013,
        };
        let adjusted = outer_for_visible(target, target, chrome_frame);
        assert_eq!(
            adjusted,
            Rect {
                left: 5,
                top: 48,
                right: 961,
                bottom: 1027
            }
        );
        assert_eq!(outer_for_visible(target, target, target), target);
        assert_eq!(
            outer_for_visible(
                target,
                target,
                Rect {
                    left: -32000,
                    ..chrome_frame
                }
            ),
            target
        );
    }
    #[test]
    fn groups_keep_two_windows_and_preserve_membership_when_a_window_closes() {
        let mut groups = Vec::new();
        for _ in 0..5 {
            groups.push(new_group(&groups));
        }
        assert_eq!(groups, vec![0, 0, 1, 1, 2]);
        groups.remove(0);
        let area = Rect {
            left: -1920,
            top: 40,
            right: 0,
            bottom: 1040,
        };
        let remaining = group_layout(area, &groups, 0, 12, 55, 0);
        let single = remaining[0].unwrap();
        assert!(((single.left - area.left) - (area.right - single.right)).abs() <= 1);
        assert_eq!(remaining.iter().flatten().count(), 1);
        assert_eq!(neighbor_group(&groups, 0, true), 1);
        assert_eq!(neighbor_group(&groups, 1, true), 3);
        assert_eq!(neighbor_group(&groups, 3, false), 1);
        assert_eq!(neighbor_group(&groups, 1, false), 0);
    }

    #[test]
    fn third_window_is_alone_in_next_group_and_fourth_joins_it() {
        let area = Rect {
            left: 0,
            top: 40,
            right: 1920,
            bottom: 1040,
        };
        let three = group_layout(area, &[0, 0, 1], 2, 12, 55, 0);
        assert!(three[0].is_none() && three[1].is_none());
        let r = three[2].unwrap();
        assert!((r.left - (area.right - r.right)).abs() <= 1);
        let four = group_layout(area, &[0, 0, 1, 1], 3, 12, 55, 0);
        let left = four[2].unwrap();
        let right = four[3].unwrap();
        assert_eq!(left.right + 12, right.left);
        assert_eq!(left.width(), right.width());
        assert_eq!(left.left, area.left + 12);
        assert_eq!(right.right, area.right - 12);
    }
    #[test]
    fn focused_visible_and_contained() {
        for left in [-1920, 0, 1920] {
            for n in 1..12 {
                for focus in 0..n {
                    for p in [20, 55, 100] {
                        let a = Rect {
                            left,
                            top: -200,
                            right: left + 1920,
                            bottom: 880,
                        };
                        let v = layout(a, n, focus, 12, p, 24);
                        assert!(v[focus].is_some());
                        for r in v.into_iter().flatten() {
                            assert!(
                                r.left >= a.left + 12
                                    && r.right <= a.right - 12
                                    && r.top >= a.top + 36
                                    && r.bottom == a.bottom - 12
                            );
                        }
                    }
                }
            }
        }
    }
    #[test]
    fn fullscreen() {
        let r = Rect {
            left: -100,
            top: 0,
            right: 900,
            bottom: 800,
        };
        assert!(covers(r, r));
        assert!(!covers(Rect { bottom: 750, ..r }, r));
    }
    #[test]
    fn tiny_and_empty() {
        assert!(layout(Rect::default(), 0, 0, 12, 55, 0).is_empty());
        assert_eq!(layout(Rect::default(), 2, 9, 12, 55, 0), vec![None, None]);
    }
    #[test]
    fn single_centered_multiple_left_aligned() {
        for left in [-1920, 0, 1920] {
            let area = Rect {
                left,
                top: 50,
                right: left + 1920,
                bottom: 1032,
            };
            let single = layout(area, 1, 0, 12, 55, 0)[0].unwrap();
            assert!(((single.left - area.left) - (area.right - single.right)).abs() <= 1);
            assert_eq!(single.width(), (area.width() - 24) * 55 / 100);
            let two = layout(area, 2, 0, 12, 40, 0);
            assert_eq!(two[0].unwrap().left, area.left + 12);
            assert_eq!(two[1].unwrap().left, two[0].unwrap().right + 12);
        }
    }
    #[test]
    fn bar_alignment_respects_work_area_and_monitor_origin() {
        for x in [-1920, 0, 1920] {
            let work = Rect {
                left: x,
                top: 50,
                right: x + 1920,
                bottom: 1032,
            };
            let bar = Rect {
                left: x + 166,
                top: 0,
                right: x + 1754,
                bottom: 50,
            };
            let area = bar_area(work, bar, 12);
            let r = layout(area, 1, 0, 12, 100, 0)[0].unwrap();
            assert_eq!(
                r,
                Rect {
                    left: bar.left,
                    top: 62,
                    right: bar.right,
                    bottom: 1020
                }
            );
            let lower = Rect { bottom: 80, ..bar };
            assert_eq!(bar_area(work, lower, 12).top, 80);
        }
    }
}
