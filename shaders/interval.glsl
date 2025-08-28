float interval_size(Interval interval) { return interval.max - interval.min; }

bool interval_contains(Interval interval, float x) { return interval.min <= x && x <= interval.max; }

bool interval_surrounds(Interval interval, float x) { return interval.min < x && x < interval.max; }