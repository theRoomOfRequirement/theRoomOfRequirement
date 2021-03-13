#include<bits/stdc++.h>
#include<stdio.h>
#include<stdlib.h>
using namespace std;

struct Server_type_info {
	string type;
	int cpu, memory, hardware_cost, daily_cost;
};

struct Server {
	string type;
	vector<list<int>> nodes;  //存储虚拟机ID 
	vector<pair<int,int>> remains;  //存储每个节点的剩余容量 
};

struct Vm_type_info {
	string type;
	int cpu, memory, on_double; 
};

struct Vm {
	int server_id;  //虚拟机部署的服务器id 
	int node;  //虚拟机部署的服务器节点（仅在虚拟机是单节点部署时使用） 
	Vm_type_info info;
};

int server_type_cnt;
unordered_map<string, Server_type_info> server_type_map;  //存储每种服务器的信息 
int vm_type_cnt;
unordered_map<string, Vm_type_info> vm_type_map;  //存储每种虚拟机的信息 
int days;
unordered_map<int, Server> server_map; //存储每个服务器的信息 
int server_index = 0; 
unordered_map<int, Vm> vm_map;  //存储使用中的每个虚拟机的信息 

void add(string vm_type, int id, vector<vector<int>>&, unordered_map<string,vector<int>>&);   
void del(int id);
void buy_server_and_set_vm(Vm& v, int id, vector<vector<int>>&, unordered_map<string,vector<int>>&);

int main() {
	scanf("%d", &server_type_cnt);
	char ch;
	for (int i = 0; i < server_type_cnt; ++i) {
		Server_type_info si;
		cin >> ch >> si.type;
		si.type.pop_back();
		scanf("%d, %d, %d, %d)", &si.cpu, &si.memory, &si.hardware_cost, &si.daily_cost);
		//cout << si.type << ' ' << si.cpu << ' ' << si.memory <<  endl;
		server_type_map[si.type] = si;
	}
	
	scanf("%d", &vm_type_cnt);
	for (int i = 0; i < vm_type_cnt; ++i) {
		Vm_type_info vi;
		cin >> ch >> vi.type;
		vi.type.pop_back();
		scanf("%d, %d, %d)", &vi.cpu, &vi.memory, &vi.on_double);
		//cout << vi.type << ' ' << vi.cpu << vi.memory << ' ' << vi.on_double << endl;
		vm_type_map[vi.type] = vi;
	}
	
	scanf("%d", &days);
	for (int i = 0; i < days; ++i) {
		int request_cnt;
		scanf("%d", &request_cnt);
		vector<vector<int>> server_and_node;  //记录当天每个虚拟机部署的服务器id和节点 
		unordered_map<string, vector<int>> daily_add;  //记录当天每种服务器的购买伪下标 
		string op_type, vm_type;
		int id;
		while(request_cnt--) {
			cin >> ch >> op_type;
			op_type.pop_back();
			if (op_type == "add") {
				cin >> vm_type >> id >> ch;
				vm_type.pop_back();
				add(vm_type, id, server_and_node, daily_add); 
			}
			else {
				cin >> id >> ch;
				del(id);
			}
		}
		
		printf("(purchase, %d)\n", daily_add.size());
		int add_cnt = 0;
		for (auto it = daily_add.begin(); it != daily_add.end(); ++it) {
			cout << '(' << it->first << ", " << it->second.size() << ")\n";
			add_cnt += it->second.size();
		}
////// 当前不考虑虚拟机迁移 
		printf("(migration, %d)\n", 0);
		
		vector<Server> fake_servers(add_cnt);
		for (int k = server_index - add_cnt; k < server_index; ++k) {
			fake_servers[k-(server_index - add_cnt)] = server_map[k];
		} 
		unordered_map<int,int> fake_to_true_index;
		int j = server_index - add_cnt;
		for (auto it = daily_add.begin(); it != daily_add.end(); ++it) {
			vector<int>& vec = it->second;
			for (int fake_index : vec) {
				Server& s = fake_servers[fake_index - (server_index - add_cnt)];
				for (list<int>& l : s.nodes) {
					for (int& vm_id : l) {
						vm_map[vm_id].server_id = j;
					}
				}
				server_map[j] = s;
				fake_to_true_index[fake_index] = j;
				j += 1;
			}
		}
		
		
		for (vector<int>& vec : server_and_node) {
			if (vec.size() == 1) {
				if (fake_to_true_index.count(vec[0]))
					printf("(%d)\n", fake_to_true_index[vec[0]]);
				else  //用的服务器id是正确的，不用转换 
					printf("(%d)\n", vec[0]);
			}
			else {
				if (fake_to_true_index.count(vec[0]))
					printf("(%d, %c)\n", fake_to_true_index[vec[0]], vec[1] + 'A');
				else
					printf("(%d, %c)\n", vec[0], vec[1] + 'A');
			}
		} 
//		cout << endl;
//		for (auto it = server_map.begin(); it != server_map.end(); ++it) {
//			for (int i = 0; i < 2; ++i) {
//				cout << char(i + 'A') << ": " << it->second.remains[i].first << ' ' << it->second.remains[i].second << endl;
//			}
//		} 
	}
	
	
} 

void add(string vm_type, int id, vector<vector<int>>& server_and_node, 
	unordered_map<string,vector<int>>& daily_add) {
	Vm v;
	v.info = vm_type_map[vm_type];
	//找到一个服务器放进去 
	bool have_place = false;
	if (v.info.on_double == 0) {
		for (auto it = server_map.begin(); it != server_map.end(); ++it) {
			for (int i = 0; i < 2; ++i) {
				pair<int,int>& cpu_memory = it->second.remains[i];
				if (cpu_memory.first >= v.info.cpu && cpu_memory.second >= v.info.memory) {
					it->second.nodes[i].push_back(id);
					cpu_memory.first -= v.info.cpu;
					cpu_memory.second -= v.info.memory;
					v.server_id = it->first;
					v.node = i;
					have_place = true;
					server_and_node.push_back({it->first,i});
					break;
				}
			}
			if (have_place) break;
		}
	}
	else {
		for (auto it = server_map.begin(); it != server_map.end(); ++it) {
			vector<pair<int,int>>& pairs = it->second.remains;
			if (pairs[0].first >= v.info.cpu/2 && pairs[0].second >= v.info.memory/2
				&& pairs[1].first >= v.info.cpu/2 && pairs[1].second >= v.info.memory/2) {
					for (int i = 0; i < 2; ++i) {
						it->second.nodes[i].push_back(id);
						pairs[i].first -= v.info.cpu/2;
						pairs[i].second -= v.info.memory/2; 
					}
					v.server_id = it->first;
					have_place = true;
					server_and_node.push_back({it->first});
					break;
				}
		}
	}
	//找不到一个现有服务器有足够空间，重新买一个并装入虚拟机 
	if (!have_place) {
		buy_server_and_set_vm(v, id, server_and_node, daily_add);
	}
	vm_map[id] = v;
}

void buy_server_and_set_vm(Vm& v, int id, vector<vector<int>>& server_and_node, 
	unordered_map<string,vector<int>>& daily_add) {
	//购买服务器用最笨的方法，找到第一个符合条件的服务器 
	for (auto it = server_type_map.begin(); it != server_type_map.end(); ++it) {
		if (v.info.on_double) {
			if (it->second.cpu >= v.info.cpu && it->second.memory >= v.info.memory) {
				Server s;
				s.type = it->first;
				for (int i = 0; i < 2; ++i) {
					s.nodes.push_back(list<int>(1,id));
					s.remains.push_back(make_pair(it->second.cpu/2-v.info.cpu/2, it->second.memory/2-v.info.memory/2));
				}	
				server_map[server_index] = s;
				v.server_id = server_index;
				server_and_node.push_back({server_index});
				daily_add[it->first].push_back(server_index);
				server_index++;
				break;
			}
		}
		else {
			if (it->second.cpu/2 >= v.info.cpu && it->second.memory/2 >= v.info.memory) {
				Server s;
				s.type = it->first;
				s.nodes.resize(2);
				s.nodes[0].push_back(id);
				s.remains.resize(2);
				s.remains[0] = make_pair(it->second.cpu/2-v.info.cpu, it->second.memory/2-v.info.memory);
				s.remains[1] = make_pair(it->second.cpu/2, it->second.memory/2);
				server_map[server_index] = s;
				v.server_id = server_index;
				v.node = 0;
				server_and_node.push_back({server_index, 0});
				daily_add[it->first].push_back(server_index);
				server_index++;
				break;
			}
		}
	}
}


void del(int id) {
	//找到所在服务器并从中移除
	Vm& v = vm_map[id];
	Server& s = server_map[v.server_id];
	if (v.info.on_double == 1) {
		for (int i = 0; i < 2; ++i) {
			s.nodes[i].remove(id);
			s.remains[i].first += v.info.cpu/2;
			s.remains[i].second += v.info.memory/2;
		}
	}
	else {
		s.nodes[v.node].remove(id);
		s.remains[v.node].first += v.info.cpu;
		s.remains[v.node].second += v.info.memory;
	}
	vm_map.erase(id); 
} 
