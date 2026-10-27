use std::collections::HashMap;
#[derive(Default)]
pub struct Spotlights {
    targets: HashMap<(isize, usize), isize>,
}
impl Spotlights {
    pub fn get(&self, monitor: isize, group: usize) -> Option<isize> {
        self.targets.get(&(monitor, group)).copied()
    }
    pub fn toggle(&mut self, monitor: isize, group: usize, target: isize) {
        if self.get(monitor, group) == Some(target) {
            self.targets.remove(&(monitor, group));
        } else {
            self.targets.insert((monitor, group), target);
        }
    }
    pub fn retain(&mut self, valid: impl Fn(isize, usize, isize) -> bool) {
        self.targets.retain(|(m, g), h| valid(*m, *g, *h));
    }
    pub fn clear(&mut self) {
        self.targets.clear();
    }
    pub fn clear_group(&mut self, monitor: isize, group: usize) {
        self.targets.remove(&(monitor, group));
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn toggles_are_local_and_survive_visiting_other_groups() {
        let mut state = Spotlights::default();
        state.toggle(1, 0, 10);
        state.toggle(1, 1, 20);
        state.toggle(2, 0, 30);
        assert_eq!(state.get(1, 0), Some(10));
        assert_eq!(state.get(1, 1), Some(20));
        assert_eq!(state.get(2, 0), Some(30));
        state.toggle(1, 0, 10);
        assert_eq!(state.get(1, 0), None);
        assert_eq!(state.get(1, 1), Some(20));
        state.retain(|_, _, h| h != 20);
        assert_eq!(state.get(1, 1), None);
        assert_eq!(state.get(2, 0), Some(30));
    }
}
