import os
import pandas as pd
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

from fitting_func import *


class Tree:
    def __init__(self, date_list, resolution=10):
        self.date_list = date_list
        self.resolution = resolution
        self.save_path = "./{}min/tree analysis".format(self.resolution)
        if not os.path.exists(self.save_path):
            os.makedirs(self.save_path)

    def average_number_of_trunks_vs_time(self):
        print("calculating average number of trunks vs time")
        trunk_number_list = []
        for date in self.date_list:
            print("calculate results of date:", date)
            df_date = pd.read_csv(
                "{}min\\jam tree trunks-resolution={}-{}.csv".format(self.resolution, self.resolution, date),
                index_col=False, engine='python')
            trunk_number_at_times = df_date['time'].values.tolist()
            trunk_number_list += trunk_number_at_times

        plt.hist(trunk_number_list, bins=24, range=(0, 1440 / self.resolution), weights=[1.0/len(self.date_list) for _ in range(len(trunk_number_list))], color='green', density=False, rwidth=0.9)  # 利用weights函数求17天的平均
        plt.xlim(0, 1440 / self.resolution)
        plt.xticks(
            [0, 360 / self.resolution, 720 / self.resolution, 1080 / self.resolution, 1440 / self.resolution],
            ["0", "6", "12", "18", "24"], fontsize=24, fontweight='bold', family='Arial')
        plt.yticks([0, 5000, 10000, 15000, 20000], ["0", "5", "10", "15", "20"], fontsize=24, fontweight='bold', family='Arial')
        plt.title('×10$^{3}$', fontsize=20, family='Arial', loc='left')
        # plt.xlabel("time to reach max cost", fontsize=12, fontweight='bold', family='Arial')
        # plt.ylabel("probability density", fontsize=12, fontweight='bold', family='Arial')
        plt.savefig(self.save_path + '/temporal number of trunks.png', dpi=300)
        plt.close()

    def total_momentary_cost_vs_time(self):
        df = pd.DataFrame(columns=['time', 'tree_cost', 'date'])
        for date in self.date_list:
            df_date = pd.read_csv("{}min\\jam tree trunks-resolution={}-{}.csv".format(self.resolution, self.resolution, date), index_col=False, engine='python')
            df_time = df_date.groupby(['time'], as_index=False)['tree_cost'].sum()
            day = [date for _ in range(len(df_time))]
            df_time['date'] = day
            df = pd.concat([df, df_time], ignore_index=True)
        df_daily_mean = df.groupby(['time'], as_index=False)['tree_cost'].mean()
        df_daily_std = df.groupby(['time'], as_index=False)['tree_cost'].std()
        x = df_daily_mean['time'].values.tolist()
        y = df_daily_mean['tree_cost'].values.tolist()
        y_err = df_daily_std['tree_cost'].values.tolist()
        lower_bound = [y[i] - y_err[i] for i in range(len(y))]
        upper_bound = [y[i] + y_err[i] for i in range(len(y))]
        plt.fill_between(x, lower_bound, upper_bound, color='green', alpha=0.5)
        plt.plot(x, y, linestyle='-', linewidth=2, color='green')
        plt.xlim(0, 1440 / self.resolution)
        plt.xticks(
            [0, 360 / self.resolution, 720 / self.resolution, 1080 / self.resolution, 1440 / self.resolution],
            ["0", "6", "12", "18", "24"], fontsize=24, fontweight='bold', family='Arial')
        plt.yticks([0, 5000, 10000, 15000, 20000], ["0", "5", "10", "15", "20"], fontsize=24, fontweight='bold', family='Arial')
        plt.title('×10$^{3}$', fontsize=20, family='Arial', loc='left')
        plt.savefig(self.save_path + '/total momentary cost of trunks vs time.png', dpi=300)
        plt.close()

    def daily_total_cost(self):
        print("calculating cost")
        df = pd.DataFrame(columns=['time', 'trunk_id', 'jam_duration', 'tree_size', 'tree_cost'])
        daily_exponent_dict = {}
        for date in self.date_list:
            df_date = pd.read_csv("{}min\\jam tree trunks-resolution={}-{}.csv".format(self.resolution, self.resolution, date), index_col=False, engine='python')
            df = pd.concat([df, df_date], ignore_index=True)
            ''' daily results '''
            df_daily_cost = df_date.groupby(['trunk_id'], as_index=False)['tree_cost'].sum()
            daily_cost_list = df_daily_cost['tree_cost'].values.tolist()
            hist, bins = np.histogram(daily_cost_list, bins=[i for i in range(int(min(daily_cost_list)), int(max(daily_cost_list) + 1))], density=True)
            x = [(bins[i] + bins[i + 1]) / 2 for i in range(len(bins) - 1)]
            y = hist
            # draw the figure
            fig, ax = plt.subplots()
            plt.scatter(x, y, c='lightgreen')
            plt.xticks(fontsize=12, fontweight='bold', family='Arial')
            plt.yticks(fontsize=12, fontweight='bold', family='Arial')
            # plt.title(str(date), fontsize=20, fontweight='bold', family='Arial')
            plt.xlabel('cost (VH)', fontsize=14, fontweight='bold', family='Arial')
            plt.ylabel('pdf', fontsize=14, fontweight='bold', family='Arial')
            plt.xscale('log')
            plt.yscale('log')
            # fitting data in tails
            x_tail, y_tail = [], []
            for i in range(len(x)):
                if x[i] >= 10:
                    x_tail.append(x[i])
                    y_tail.append(y[i])
            popt, pcov = curve_fit(func_power_law, x_tail, y_tail)
            y_fitted = [func_power_law(i, *popt) for i in x_tail]
            a, b = popt[0], popt[1]
            daily_exponent_dict[date] = abs(b)
            plt.plot(x_tail, y_fitted, color="black", linewidth=3, linestyle='--')
            plt.text(0.6, 0.8, r'$\beta$ = ' + str(round(b, 2)), transform=ax.transAxes, fontsize=20, fontweight='bold',
                     family='Arial')
            plt.text(0.1, 0.1, '{}'.format(date), transform=ax.transAxes, fontsize=20, fontweight='bold', family='Arial')
            plt.savefig(self.save_path + '/daily total cost-{}.png'.format(date), dpi=300)
            plt.close()
        ''' histogram '''
        mean, std = np.mean(list(daily_exponent_dict.values())), np.std(list(daily_exponent_dict.values()))
        plt.hist(list(daily_exponent_dict.values()), bins=20, range=[1.5, 2.5], color='lightgreen', density=True, rwidth=0.9)
        plt.xticks(fontsize=14, fontweight='bold', family='Arial')
        plt.yticks(fontsize=14, fontweight='bold', family='Arial')
        plt.title(r'$\beta$ = %.2f ± %.2f' % (mean, std), fontsize=20, fontweight='bold', family='Arial')
        plt.xlabel(r'$\beta$', fontsize=14, fontweight='bold', family='Arial')
        plt.ylabel('pdf', fontsize=14, fontweight='bold', family='Arial')
        plt.savefig(self.save_path + '/daily total cost-pdf of exponents.png', dpi=300)
        plt.close()
        ''' save results '''
        exponent_list = [(date, round(daily_exponent_dict[date], 2)) for date in daily_exponent_dict.keys()]
        df_expo = pd.DataFrame(exponent_list, columns=['date', 'exponent'])
        df_expo.to_csv(self.save_path + '/daily total cost exponent.csv', index=False)

    def evolution_of_cost(self):
        # set time window as 20min for {Beijing, Shenzhen, Beijing-Tianjin-Hebei}, and 30min for other three areas
        time_window = int(20 / self.resolution)
        start_time = int(0 / self.resolution)
        end_time = int(1440 / self.resolution)
        morning_rush_exponent, evening_rush_exponent, non_rush_exponent = [], [], []
        fig, ax = plt.subplots()
        for date in self.date_list:
            print("calculate results of date:", date)
            exponent_dict = {}
            df = pd.read_csv(
                "{}min\\jam tree trunks-resolution={}-{}.csv".format(self.resolution, self.resolution, date),
                index_col=False, engine='python')
            for time in range(start_time, end_time, time_window):
                df_window = df.loc[(df['time'] >= time) & (df['time'] < time + time_window)]
                df_window.reset_index(drop=True, inplace=True)
                df_grouped = df_window.groupby(['trunk_id'], as_index=False)['tree_cost'].sum()
                cost_list = df_grouped['tree_cost'].values.tolist()
                hist, bins = np.histogram(cost_list,
                                          bins=[i for i in range(int(min(cost_list)), int(max(cost_list) + 1))],
                                          density=True)
                x = [(bins[i] + bins[i + 1]) / 2 for i in range(len(bins) - 1)]
                y = hist
                x_tail, y_tail = [], []
                for i in range(len(x)):
                    if x[i] >= 1:
                        x_tail.append(x[i])
                        y_tail.append(y[i])
                popt, pcov = curve_fit(func_power_law, x_tail, y_tail)
                a, b = popt[0], popt[1]
                exponent_dict[time] = abs(b)
            plt.plot(list(exponent_dict.keys()), list(exponent_dict.values()), linestyle='-', linewidth=0.5,
                     color='blue')
            morning_rush_exponent.append(exponent_dict[int(480 / self.resolution)])
            evening_rush_exponent.append(exponent_dict[int(1080 / self.resolution)])
            non_rush_exponent.append(exponent_dict[int(720 / self.resolution)])

        plt.xticks([0, 360 / self.resolution, 720 / self.resolution, 1080 / self.resolution, 1440 / self.resolution],
                   ["0", "6", "12", "18", "24"], fontsize=24, fontweight='bold', family='Arial')
        plt.xlim(0, 1440 / self.resolution)
        plt.yticks(fontsize=24, fontweight='bold', family='Arial')
        y_start, y_end = 1.1, 2.5
        plt.ylim(y_start, y_end)
        plt.plot([480 / self.resolution, 480 / self.resolution], [y_start, y_end], linestyle='--', linewidth=3,
                 color='red')
        plt.plot([1080 / self.resolution, 1080 / self.resolution], [y_start, y_end], linestyle='--', linewidth=3,
                 color='red')
        plt.plot([720 / self.resolution, 720 / self.resolution], [y_start, y_end], linestyle='--', linewidth=3,
                 color='green')
        plt.text(0.2, 0.05,
                 str(round(np.mean(morning_rush_exponent), 2)) + '±' + str(round(np.std(morning_rush_exponent), 2)),
                 transform=ax.transAxes, fontsize=20, fontweight='bold', family='Arial')
        plt.text(0.6, 0.05,
                 str(round(np.mean(evening_rush_exponent), 2)) + '±' + str(round(np.std(evening_rush_exponent), 2)),
                 transform=ax.transAxes, fontsize=20, fontweight='bold', family='Arial')
        plt.text(0.4, 0.85,
                 str(round(np.mean(non_rush_exponent), 2)) + '±' + str(round(np.std(non_rush_exponent), 2)),
                 transform=ax.transAxes, fontsize=20, fontweight='bold', family='Arial')
        # plt.xlabel('time', fontsize=12, fontweight='bold', family='Arial')
        # plt.ylabel('exponent', fontsize=12, fontweight='bold', family='Arial')
        plt.savefig(self.save_path + '/momentary jam tree cost_evolution.png', dpi=300)
        plt.close()