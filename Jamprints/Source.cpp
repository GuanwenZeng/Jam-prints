#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <stack>
#include <algorithm>
#include <cmath>
#include <iomanip>
using namespace std;
const int LINKNUM = 11; // number of links in the network, based on the input file
const int NODENUM = 12; // number of nodes in the network, based on the input file
const int TIME_WINDOW_NUM = 12; // number of time window. Each time window represents 10 min in this case
double Weight[LINKNUM][TIME_WINDOW_NUM] = { 0 }; // weight matrix. Weight of each link at each time
int Connection[LINKNUM][TIME_WINDOW_NUM] = { 0 }; // connection state associated to a tree, derived by relative velocity
int JamDurationMatrix[LINKNUM][TIME_WINDOW_NUM] = { 0 }; // the successive time interval that a road segment has been congested
double Cost[LINKNUM][TIME_WINDOW_NUM] = { 0 }; // jam cost


class Edge
{
public:
	int link_id; // road id
	int from_id; // road source id
	int to_id; // road target id
	string name; // road name
	double length; // road length
	Edge* next;
	int link_index; // reset road id, start from 0
	int from; // reset source id, start from 0
	int to; // reset target id, start from 0
	double jam_threshold; // weight threshold. Checking if a road is congested
	bool congested; // indicate a road is congested or not
	bool bfs_visited; // indicate a link is visited or not
	int tree_size; // size of a tree that this link belongs to
	double tree_cost; // cost of a tree that this link belongs to
	vector<int> belong_to_trunk; // trunk of a tree that this link belongs to (which could be multiple)
	vector<int> upstream; // upstream roads
	vector<int> downstream; // downstream roads
	Edge() {
		from = -1;
		to = -1;
		jam_threshold = 0.5;
		congested = false;
		tree_size = 0;
		tree_cost = 0;
		next = nullptr;
		belong_to_trunk.clear();
		upstream.clear();
		downstream.clear();
	}
	~Edge() {}
} Link[LINKNUM];
class Vertex
{
public:
	int node_id; // intersection id
	Edge* first;
	Vertex() {
		node_id = -1;
		first = nullptr;
	}
	~Vertex() {}
} Node[NODENUM];
class Network
{
public:
	void create_network();
	void get_weight_matrix();
	void calculate_connection_matrix();
	void calculate_jam_duration_matrix();
	void calculate_cost_matrix();
	void find_jam_trees();

	void jam_tree_analysis();
};


void Network::create_network()
{
	cout << "input and create network structure" << endl;
	ifstream linkin("input files\\demo network.txt"); // name of input file
	for (int i = 0; i < LINKNUM; i++)
	{
		linkin >> Link[i].link_id >> Link[i].from_id >> Link[i].to_id >> Link[i].name >> Link[i].length;
	}
	Edge* p;
	for (int i = 0; i < LINKNUM; i++)
	{
		p = &Link[i];
		Link[i].link_index = i;
		for (int j = 0; j < NODENUM; j++)
		{
			if (Node[j].node_id == Link[i].from_id)
			{
				Link[i].from = j;
				break;
			}
			else if (Node[j].node_id == -1)
			{
				Node[j].node_id = Link[i].from_id;
				Link[i].from = j;
				break;
			}
		}
		p->next = Node[p->from].first;
		Node[p->from].first = p;
		for (int m = 0; m < NODENUM; m++)
		{
			if (Node[m].node_id == Link[i].to_id)
			{
				Link[i].to = m;
				break;
			}
			else if (Node[m].node_id == -1)
			{
				Node[m].node_id = Link[i].to_id;
				Link[i].to = m;
				break;
			}
		}
	}
	// check upstream and downstream roads
	for (int i = 0; i < LINKNUM; ++i)
	{
		int node_to_id = Link[i].to;
		Edge* temp = Node[node_to_id].first;
		while (temp != nullptr)
		{
			if (Link[i].from != Link[temp->link_index].to)
			{
				Link[temp->link_index].upstream.push_back(i);
				Link[i].downstream.push_back(temp->link_index);
			}
			temp = temp->next;
		}
	}
}
void Network::get_weight_matrix()
{
	/*
		we directly input the weight of each link at each time. Calculating weight of a link could be based on the real-data velocity information 
	*/
	ifstream input_weight("input files\\demo weight matrix.txt"); // input file for this demo case
	for (int time = 0; time < TIME_WINDOW_NUM; ++time)
	{
		for (int i = 0; i < LINKNUM; ++i)
		{
			input_weight >> Weight[i][time];
		}
	}
}
void Network::calculate_connection_matrix()
{
	for (int time = 0; time < TIME_WINDOW_NUM; ++time)
	{
		for (int i = 0; i < LINKNUM; ++i)
		{
			if (Weight[i][time] > 0) // for effective information
			{
				if (Weight[i][time] < Link[i].jam_threshold) {
					Connection[i][time] = 1;
				}
			}
			else // for vacant information
			{
				int upstream_jam_count = 0;
				int downstream_jam_count = 0;
				// for upstream
				for (vector<int>::iterator up_iter = Link[i].upstream.begin(); up_iter != Link[i].upstream.end(); ++up_iter)
				{
					if (Weight[*up_iter][time] > 0 && Weight[*up_iter][time] < Link[*up_iter].jam_threshold)
					{
						++upstream_jam_count;
					}
				}
				// for downstream
				for (vector<int>::iterator down_iter = Link[i].downstream.begin(); down_iter != Link[i].downstream.end(); ++down_iter)
				{
					if (Weight[*down_iter][time] > 0 && Weight[*down_iter][time] < Link[*down_iter].jam_threshold)
					{
						++downstream_jam_count;
					}
				}
				if (upstream_jam_count > 0 && downstream_jam_count > 0)
				{
					Connection[i][time] = 1;
				}
			}
		}
	}
}
void Network::calculate_jam_duration_matrix()
{
	for (int i = 0; i < LINKNUM; ++i)
	{
		// timestamp = 0
		if (Connection[i][0] == 1)
			JamDurationMatrix[i][0] = 1;
		else
			JamDurationMatrix[i][0] = 0;
		// timestamp > 0, consideration of temporal relations are needed
		for (int t = 1; t < TIME_WINDOW_NUM; ++t)
		{
			if (Connection[i][t] == 1)
				JamDurationMatrix[i][t] = JamDurationMatrix[i][t - 1] + 1;
			else
				JamDurationMatrix[i][t] = 0;
		}
	}
}
void Network::calculate_cost_matrix()
{
	const double m = 0.8;
	const double l = 2.8;
	const double k_j = 150; // jam density (unit:veh/km)
	for (int tw = 0; tw < TIME_WINDOW_NUM; ++tw)
	{
		for (int i = 0; i < LINKNUM; ++i)
		{
			double u_f = 100; // suppose free-flow velocity as 50 km/h
			double u = u_f * Weight[i][tw];
			if (u > u_f) {
				u = u_f;
			}
			double u_op = 50; // suppose optimal velocity as 50 km/h
			if (u_f > 0 && u > 0 && u_op > 0)
			{
				double k = k_j * pow((1 - pow(u / u_f, 1 - m)), 1 / (l - 1));
				double q = k * u;
				// calculating cost
				double dist_ij = Link[i].length / 1000; // transfer unit to kilometer
				int l = 3; // suppose 3 lanes for the demo case
				int T = 10; // 10 min for each time window
				Cost[i][tw] = dist_ij * (1 / u - 1 / u_op) * (q * l / (60 / T));
				/*
				Cost[i][tw] = 1; // for more simplified case, we can assume that Cost[i][tw] is always 1 to test the algorithm.
				*/ 
			}
			else
			{
				Cost[i][tw] = -1;
			}
		}
	}
}
void Network::find_jam_trees()
{
	const int theta = 2; // temporal difference for judging if two adjacent links belongs to the same tree
	ofstream tree_out("demo results\\demo jam tree trunks-resolution=10min.csv"); // output results
	tree_out << "time" << ',' << "trunk_id" << ',' << "jam_duration" << ',' << "tree_size" << ',' << "tree_cost" << endl;
	for (int time = 0; time < TIME_WINDOW_NUM; ++time)
	{
		// initialization
		for (int i = 0; i < LINKNUM; ++i)
		{
			Link[i].congested = false;
			Link[i].bfs_visited = false;
			Link[i].tree_size = 0;
			Link[i].tree_cost = 0;
			Link[i].belong_to_trunk.clear();
			if (JamDurationMatrix[i][time] > 0) {
				Link[i].congested = true;
			}
		}
		vector<int> trunk_set;
		trunk_set.clear();
		// determine whether a link is a trunk or not
		for (int i = 0; i < LINKNUM; ++i)
		{
			if (Link[i].congested == true)
			{
				bool is_trunk = true;
				for (vector<int>::iterator iter = Link[i].downstream.begin(); iter != Link[i].downstream.end(); ++iter)
				{
					if (JamDurationMatrix[*iter][time] - JamDurationMatrix[i][time] <= theta && JamDurationMatrix[*iter][time] - JamDurationMatrix[i][time] >= 0)
					{
						is_trunk = false;
						break;
					}
				}
				if (is_trunk) {
					trunk_set.push_back(i);
				}
			}
		}
		// find jam tree members based on BFS
		queue<int> Q;
		for (vector<int>::iterator iter = trunk_set.begin(); iter != trunk_set.end(); ++iter)
		{
			for (int i = 0; i < LINKNUM; ++i) {
				Link[i].bfs_visited = false;
			}
			while (!Q.empty())
				Q.pop();
			// search from a trunk
			int current_trunk = *iter;
			Link[current_trunk].bfs_visited = true;
			Q.push(current_trunk);
			Link[current_trunk].belong_to_trunk.push_back(current_trunk);
			while (!Q.empty())
			{
				int w = Q.front();
				for (vector<int>::iterator up_iter = Link[w].upstream.begin(); up_iter != Link[w].upstream.end(); ++up_iter)
				{
					if (Link[*up_iter].bfs_visited == false && Link[*up_iter].congested == true) {
						if (JamDurationMatrix[w][time] - JamDurationMatrix[*up_iter][time] >= 0 && JamDurationMatrix[w][time] - JamDurationMatrix[*up_iter][time] <= theta)
						{
							Link[*up_iter].bfs_visited = true;
							Q.push(*up_iter);
							Link[*up_iter].belong_to_trunk.push_back(current_trunk);
						}
					}
				}
				Q.pop();
			}
		}
		for (int i = 0; i < LINKNUM; ++i)
		{
			if (Link[i].congested == true && Link[i].belong_to_trunk.empty() == false)
			{
				for (vector<int>::iterator iter = Link[i].belong_to_trunk.begin(); iter != Link[i].belong_to_trunk.end(); ++iter)
				{
					++Link[*iter].tree_size;
					if (Cost[i][time] > 0)
						Link[*iter].tree_cost += (Cost[i][time] / Link[i].belong_to_trunk.size()); // averaged by each trunk it belongs to
				}
			}
		}
		for (vector<int>::iterator iter = trunk_set.begin(); iter != trunk_set.end(); ++iter)
		{
			if (Link[*iter].tree_size > 1)
			{
				tree_out << time << ',' << Link[*iter].link_index << ',' << JamDurationMatrix[*iter][time] << ',' << Link[*iter].tree_size << ',' << Link[*iter].tree_cost << endl;
			}
		}
	}
}

void Network::jam_tree_analysis()
{
	// initialization
	for (int i = 0; i < LINKNUM; ++i) {
		for (int tw = 0; tw < TIME_WINDOW_NUM; ++tw) {
			Connection[i][tw] = 0;
			JamDurationMatrix[i][tw] = 0;
			Cost[i][tw] = 0;
		}
	}
	// calculation
	get_weight_matrix();
	calculate_connection_matrix();
	calculate_jam_duration_matrix();
	calculate_cost_matrix();

	find_jam_trees();
}




int main()
{
	Network* N = new Network;
	N->create_network();
	N->jam_tree_analysis();
	delete N;

	return 0;
}