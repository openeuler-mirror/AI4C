import sys
import math
import heapq
from typing import List
from random import randint, random
import logging

logging.basicConfig(format='INFO: %(message)s',
                    level=logging.INFO,
                    handlers=[logging.StreamHandler(sys.stdout)])


class SimulatedAnnealingOptimizer(object):

    def __init__(self, parallel_size, t4, t4_defaults, t4_minv, t4_maxv,
                 search_len):
        self.parallel_size = parallel_size
        self.points = None
        self.n_iter = 500
        self.temp = (1, 0)
        self.len = search_len
        self.persistent = True
        self.t4, self.t4_defaults, self.t4_minv, self.t4_maxv = t4, t4_defaults, t4_minv, t4_maxv

    def sample_inits(self):
        # Initial bool config is all zero.
        return [0 for i in range(self.len)]

    def sample_inits_t4(self):
        return [
            randint(int(self.t4_minv[i]), int(self.t4_maxv[i]))
            for i in range(len(self.t4))
        ]

    def find_maxima(self, model, num, exclusive):
        """ search process

        Do the SA search, find points with the best performance based on 
        the xgb cost model.

        Args:
            model(class XGBoostCostModel): The cost model used for prediction.
            num(int): The number of maximum points to find.
            exclusive(set): A set of points that should not be included in the 
            maximum points.

        Returns:
            maxima: A list of points with the best performance.

        """
        if self.parallel_size and self.points is not None:
            points = self.points
        else:
            points = []
            for _ in range(self.parallel_size):
                points.append(self.sample_inits() + self.sample_inits_t4())

        # Put the default value of k4 as the first point,
        # and start the search from the default value for one point each time
        points[0] = self.sample_inits() + [
            int(self.t4_defaults[i]) for i in range(len(self.t4_defaults))
        ]
        scores = model.predict(points)

        # Build heap and insert initial points.
        initial_list = [100 for i in range(self.len + len(self.t4))]
        heap_items = [(-1000, [j + i for j in initial_list])
                      for i in range(num)]
        heapq.heapify(heap_items)
        in_heap = set(exclusive)
        in_heap.update([hash(str(x[1])) for x in heap_items])

        # Compare the current score with the maximum value in the heap,
        # if it is smaller than the maximum value, then add it to the heap.
        for scr, pnt in zip(scores, points):
            if scr < -heap_items[0][0] and hash(str(pnt)) not in in_heap:
                pop = heapq.heapreplace(heap_items, (-scr, pnt))
                in_heap.remove(hash(str(pop[1])))
                in_heap.add(hash(str(pnt)))
        # Initialize the iteration count
        k = 0
        # Initialize the temperature
        t = float(self.temp[0] * self.n_iter)
        cooling_rate = 0.98

        while k < self.n_iter and t > 0:
            new_points = list(points)
            for i, pnt in enumerate(points):
                new_points[i] = self.random_walk(pnt)

            new_scores = model.predict(new_points)

            for i, score in enumerate(scores):
                # `de` is the score difference between the new point and the
                # current point. The score usually refers to the runtime of a
                # program.
                de = new_scores[i] - scores[i]
                if de < 0:  # if new < old
                    points[i] = new_points[i]
                    scores[i] = new_scores[i]
                else:  # accept bad result by Metropolis
                    try:
                        dt = de / t
                    except ZeroDivisionError as err:
                        logging.info(f"{err}!")
                        break
                    else:
                        if math.exp(-dt) < random():
                            points[i] = new_points[i]
                            scores[i] = new_scores[i]

            for scr, pnt in zip(new_scores, new_points):
                if scr < -heap_items[0][0] and hash(str(pnt)) not in in_heap:
                    pop = heapq.heapreplace(heap_items, (-scr, pnt))
                    in_heap.remove(hash(str(pop[1])))
                    in_heap.add(hash(str(pnt)))

            k += 1
            t = t * cooling_rate

        heap_items.sort(key=lambda item: -item[0])
        heap_items = [x for x in heap_items if -x[0] < 1000]

        if self.persistent:
            self.points = points

        return [x[1] for x in heap_items]

    def random_walk(self, point: List[int]):
        """Perform one step of random walk.

        Generate a new point of compiler options based on the old point.
        Randomly pick one index of the compiler option list and randomly generate
        a compiler option value for this compiler option based on its value range
        while keeping all other compiler option values the same.

        Args:
            point (obj:`list` of obj:`int`): A list of compiler option values.

        Returns:
            new_point (obj:`list` of obj:`int`): A new list of compiler option
                values.

        """
        old_point = point
        new_point = list(old_point)
        while new_point == old_point:
            index = randint(0, len(old_point) - 1)
            if index < self.len:
                value = randint(0, 1)
                new_point[index] = value
            else:
                new_point_index = index - self.len
                value = randint(int(self.t4_minv[new_point_index]),
                                int(self.t4_maxv[new_point_index]))
                new_point[index] = value
        return new_point


if __name__ == '__main__':
    pass
