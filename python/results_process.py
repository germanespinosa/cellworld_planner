import pandas as pd
import io


def load_csv(depth):
    file_name = 'clustering_%03d.csv' % depth
    adf = pd.read_csv(file_name)
    ldf = adf[adf.sim_type == "LPPO"]
    sdf = ldf.sort_values(by=['lppo_benefit'], ascending=False)
    bdf = sdf[0:200]
    sdf = ldf.sort_values(by=['lppo_benefit'], ascending=True)
    wdf = sdf[0:200]
    return depth, adf, ldf, bdf, wdf


def get_intersect(argv):
    df = argv[0]
    for mdf in argv[1:]:
        df = pd.merge(df[["world_number", "entropy_bucket"]], mdf[["world_number", "entropy_bucket"]], how='inner')
    return df


def get_average_benefit(filter_df, argv):
    filtered = []
    for mdf in argv:
        filtered.append(pd.merge(filter_df[["world_number", "entropy_bucket"]], mdf, how='inner'))
    fdf = pd.concat(filtered)
    df = fdf[["world_number", "entropy_bucket", 'lppo_benefit']].groupby(["world_number", "entropy_bucket"]).mean()
    return df


def show_benefit(df):
    df = df[["world_number", "entropy_bucket", 'lppo_benefit']].groupby(["world_number", "entropy_bucket"]).mean()
    entropy_bucket_value = df.groupby("entropy_bucket").mean()
    entropy_bucket_value.plot()
    print(entropy_bucket_value)


depths = [25, 50, 100, 200]

datasets = []
for depth in depths:
    datasets.append(load_csv(depth))

best_worlds = get_intersect([b for d, r, l, b, w in datasets])
worst_worlds = get_intersect([w for d, r, l, b, w in datasets])
best_worlds_benefit = get_average_benefit(best_worlds, [b for d, r, l, b, w in datasets])
worst_worlds_benefit = get_average_benefit(worst_worlds, [w for d, r, l, b, w in datasets])

print("BEST worlds")
print("--------------")
for d, r, l, b, w in datasets:
    print("depth:", d)
    print("--------------")
    show_benefit(b)
    print("")

print("--------------")
print("--------------")
print(best_worlds_benefit)
print(best_worlds.groupby("entropy_bucket").count())
print("--------------")
print("--------------")

print("WORST worlds")
print("--------------")
for d, r, l, b, w in datasets:
    print("depth:", d)
    print("--------------")
    show_benefit(w)
    print("")

print("--------------")
print("--------------")
print(worst_worlds_benefit)
print(worst_worlds.groupby("entropy_bucket").count())
print("--------------")
print("--------------")

