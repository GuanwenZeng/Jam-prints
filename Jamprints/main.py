from trees import Tree


if __name__ == "__main__":
    resolution = 10
    # data_list is different for different areas
    date_list = [20151008, 20151009, 20151010, 20151012, 20151013, 20151014, 20151015, 20151016, 20151020, 20151021,
                 20151022, 20151023, 20151026, 20151027, 20151028, 20151029, 20151030]

    tree_analysis = Tree(date_list=date_list, resolution=resolution)
    tree_analysis.average_number_of_trunks_vs_time()
    tree_analysis.total_momentary_cost_vs_time()
    tree_analysis.daily_total_cost()
    tree_analysis.evolution_of_cost()