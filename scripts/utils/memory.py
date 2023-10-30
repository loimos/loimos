import sys
import pandas as pd

# Memory profiling code taken from:
# https://towardsdatascience.com/python-garbage-collection-article-4a530b0992e3


def obj_size_fmt(num):
    if num < 10 ** 3:
        return "{:.2f}{}".format(num, "B")  # noqa
    elif (num >= 10 ** 3) & (num < 10 ** 6):  # noqa
        return "{:.2f}{}".format(num / (1.024 * 10 ** 3), "KB")
    elif (num >= 10 ** 6) & (num < 10 ** 9):  # noqa
        return "{:.2f}{}".format(num / (1.024 * 10 ** 6), "MB")  # noqa
    else:
        return "{:.2f}{}".format(num / (1.024 * 10 ** 9), "GB")  # noqa


def memory_usage(all_vars=None):
    if all_vars is None:
        all_vars = globals()
    memory_usage_by_variable = pd.DataFrame(
        {k: sys.getsizeof(v) for (k, v) in globals().items()}, index=["Size"]
    )

    memory_usage_by_variable = memory_usage_by_variable.T
    memory_usage_by_variable = memory_usage_by_variable.sort_values(
        by="Size", ascending=False
    ).head(10)
    memory_usage_by_variable["Size"] = memory_usage_by_variable["Size"].apply(
        lambda x: obj_size_fmt(x)
    )

    return memory_usage_by_variable
