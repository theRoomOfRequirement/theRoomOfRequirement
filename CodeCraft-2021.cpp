#include<bits/stdc++.h>
#include<stdio.h>
#include<stdlib.h>
using namespace std;

/*
v1.5
当前程序流程是：
	1.处理add和del请求，记录下服务器和虚拟机变动1
	2.处理migrate请求，记录下服务器和虚拟机变动2（因为迁移会改变虚拟机位置，且有机会导致少买服务器）
		如果需要迁移的是当日新增的虚拟机，则直接改变装入位置，不计入迁移数目（已实现）
	3.整合服务器和虚拟机变动1,2，输出扩容请求
	4.输出migrate请求
	5.将伪服务器下标转换为实际下标
*/

struct Server_type_info {	// 菜单上的某服务器的信息
	string type;			// 服务器型号
	int cpu, memory, hardware_cost, daily_cost;	// cpu核数，内存大小，硬件成本，每日能耗成本
};

struct Server {						// 已购入的某服务器的信息
	string type;					// 服务器型号
	vector<list<int>> nodes;		// 该服务器装了哪些虚拟机，存储虚拟机ID 
	vector<pair<int,int>> remains;	// 服务器两个结点剩余cpu核数、内存大小，remains存储每个节点的剩余容量，pair一个存CPU核数，一个存内存大小
};

struct Vm_type_info {	// 菜单上的某虚拟机的信息
	string type;		// 虚拟机型号
	int cpu, memory, on_double; 
};

struct Vm {			// 已购入的某虚拟机的信息
	int server_id;  //虚拟机部署的服务器id 
	int node;  		//虚拟机部署的服务器节点（仅在虚拟机是单节点部署时使用） 
	Vm_type_info info;
};

struct migration_order {			// 存储某虚拟机迁移请求的目标服务器信息
	int target_ID;		// 迁移目标服务器ID
	int target_node;	// 迁移到服务器的哪个节点上（仅在虚拟机是单节点部署时使用） 
	int on_double;		// 虚拟机是否是双节点部署的
};

int server_type_cnt;				// 服务器类型数
unordered_map<string, Server_type_info> server_type_map;	//存储菜单上的每种服务器的信息，first是服务器型号
int vm_type_cnt;					// 虚拟机类型数
unordered_map<string, Vm_type_info> vm_type_map;			//存储菜单上的每种虚拟机的信息，first是虚拟机型号
int days;							// 总共多少天
vector<Server> servers;				// 存储已购的每个服务器的信息，下标是服务器对应的ID
int server_index = 0;				// servers结尾下标
unordered_map<int, Vm> vm_map;		// 存储使用中的每个虚拟机的信息，first是虚拟机ID
int vm_installed_cnt;				// 已经装了的虚拟机数量

void add(string vm_type, int id, vector<vector<int>>&, unordered_map<string,vector<int>>&);	// 创建虚拟机申请
void del(int id);	// 删除虚拟机申请
void buy_server_and_set_vm(Vm& v, int id, vector<vector<int>>&, unordered_map<string,vector<int>>&);	// 购买服务器并将导致超载的虚拟机装入
void migration(unordered_map<int, migration_order>& daily_migration, 
	vector<int>& new_v, unordered_map<string, vector<int>>& daily_add, int vm_installed_cnt);	// 处理虚拟机迁移请求
int get_vm_amount(Server server);	// 返回服务器上装了的虚拟机数目

int main() {
	char ch;
	vm_installed_cnt = 0;

	// 输入服务器菜单
	scanf("%d", &server_type_cnt);
	for (int i = 0; i < server_type_cnt; ++i) {
		Server_type_info si;
		cin >> ch >> si.type;
		si.type.pop_back();
		scanf("%d, %d, %d, %d)", &si.cpu, &si.memory, &si.hardware_cost, &si.daily_cost);
		server_type_map[si.type] = si;
	}
	
	// 输入虚拟机菜单
	scanf("%d", &vm_type_cnt);
	for (int i = 0; i < vm_type_cnt; ++i) {
		Vm_type_info vi;
		cin >> ch >> vi.type;
		vi.type.pop_back();
		scanf("%d, %d, %d)", &vi.cpu, &vi.memory, &vi.on_double);
		vm_type_map[vi.type] = vi;
	}
	
	// 输入日期和请求并处理
	scanf("%d", &days);
	for (int i = 0; i < days; ++i) {	// 遍历日期
		int request_cnt;	// add和del请求数量
		vector<vector<int>> server_and_node;  //记录当天每个虚拟机部署的服务器id和节点 
		unordered_map<string, vector<int>> daily_add;  //记录当天每种服务器的购买伪下标。first是服务器型号，second是该型号服务器新增服务器的下标列表
		vector<int> new_vm;	// 记录当天新创建的虚拟机ID
		string op_type, vm_type;
		int id;
		int add_cnt = 0;	// 当天新买的服务器数量
		int vm_add_cnt = 0;	// 当天新装的虚拟机数量每天开始时都清0

		scanf("%d", &request_cnt);
		while(request_cnt--) {	//处理创建和删除申请，并首次记录扩容请求
			cin >> ch >> op_type;
			op_type.pop_back();
			if (op_type == "add") {
				cin >> vm_type >> id >> ch;
				vm_type.pop_back();
				add(vm_type, id, server_and_node, daily_add);	// 处理创建申请
				new_vm.push_back(id);
				vm_add_cnt++;
			}
			else {
				cin >> id >> ch;
				del(id);	// 处理删除申请
				vm_add_cnt--;
			}
		}

		// 处理虚拟机迁移，并第二次记录扩容请求
		unordered_map<int, migration_order> daily_migration;  // 记录当天虚拟机的迁移请求，first是虚拟机id，second记录剩下的信息
		migration(daily_migration, new_vm, daily_add, vm_installed_cnt);	// 处理当天的迁移请求
		
		// 处理完迁移请求再增加已装虚拟机的数量（因为迁移是在装机前发生的，vm_installed_cnt会影响当日可迁移次数）
		vm_installed_cnt += vm_add_cnt;
		
		// 输出扩容请求
		printf("(purchase, %d)\n", daily_add.size());
		for (auto it = daily_add.begin(); it != daily_add.end(); ++it) {
			cout << '(' << it->first << ", " << it->second.size() << ")\n";
			add_cnt += it->second.size();
		}

		// 输出迁移请求
		printf("(migration, %d)\n", daily_migration.size());
		for (auto it = daily_migration.begin(); it != daily_migration.end(); ++it) {
			cout << '(' << it->first << ", " << it->second.target_ID;
			if (it->second.on_double == 1)
				cout <<")\n";
			else
			{
				cout << ", ";
				if (it->second.target_node==0)
					cout << "A";
				else
					cout << "B";
				cout << ")\n";
			}
		}

////// 下面代码的作用就是把伪服务器下标转换为实际下标（从按照购买顺序存储转换为按照输出中的申请顺序存储） 
		vector<Server> fake_servers(add_cnt);	// 伪服务器列表，下标为伪服务器下标，下标从0开始
		for (int k = server_index - add_cnt; k < server_index; ++k) {	// 遍历所有当日添加的服务器，放到fake_servers中（fake_servers下标从0开始）
			fake_servers[k-(server_index - add_cnt)] = servers[k];
		} 
		unordered_map<int,int> fake_to_true_index;	// fake_index和true_index一一对应
		int j = server_index - add_cnt;
		for (auto it = daily_add.begin(); it != daily_add.end(); ++it) {
			vector<int>& vec = it->second;	// 特定型号服务器新增服务器的下标列表
			for (int fake_index : vec) {
				Server& s = fake_servers[fake_index - (server_index - add_cnt)];
				for (list<int>& l : s.nodes) {
					for (int& vm_id : l) {
						vm_map[vm_id].server_id = j;	// 修改虚拟机装入的服务器id
					}
				}
				servers[j] = s;
				fake_to_true_index[fake_index] = j;
				j += 1;
			}
		}
		
		// 输出创建请求
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
	}
} 

void add(string vm_type, int id, vector<vector<int>>& server_and_node, 
	unordered_map<string,vector<int>>& daily_add) {
	Vm v;
	v.info = vm_type_map[vm_type];
	//找到一个服务器放进去 
	//v1.1,用贪心算法，从所有可选服务器中选择剩余空间最小的服务器 
	//定义空间大小 = cpu + 内存 结果2121936153 
	bool have_place = false;
	if (v.info.on_double == 0) {	// 单节点VM
		int index, node, min_cpu_and_momory_remained = INT_MAX;	// 剩余空间最小服务器的信息
		for (int j = 0; j < servers.size(); ++j) {
			for (int i = 0; i < 2; ++i) {	// 0代表A结点，1代表B结点
				pair<int,int>& cpu_memory = servers[j].remains[i];
				if (cpu_memory.first >= v.info.cpu && cpu_memory.second >= v.info.memory
					&& cpu_memory.first + cpu_memory.second < min_cpu_and_momory_remained) {
						index = j;
						node = i;
						have_place = true;
				}
			}
		}
		if (have_place) {
			pair<int,int>& cpu_memory = servers[index].remains[node];
			servers[index].nodes[node].push_back(id);
			cpu_memory.first -= v.info.cpu;
			cpu_memory.second -= v.info.memory;
			v.server_id = index;
			v.node = node;
			server_and_node.push_back({index,node});
		}
	}
	else {	// 双节点VM
		int index, min_cpu_and_momory_remained = INT_MAX;
		for (int j = 0; j < servers.size(); ++j) {
			vector<pair<int,int>>& pairs = servers[j].remains;
			if (pairs[0].first >= v.info.cpu/2 && pairs[0].second >= v.info.memory/2
				&& pairs[1].first >= v.info.cpu/2 && pairs[1].second >= v.info.memory/2
				&& pairs[0].first + pairs[0].second + pairs[1].first + pairs[1].second < min_cpu_and_momory_remained ) {
					index = j;
					have_place = true;
				}
		}
		if (have_place) {
			vector<pair<int,int>>& pairs = servers[index].remains;
			for (int i = 0; i < 2; ++i) {
				servers[index].nodes[i].push_back(id);
				pairs[i].first -= v.info.cpu/2;
				pairs[i].second -= v.info.memory/2; 
			}
			v.server_id = index;
			have_place = true;
			server_and_node.push_back({index});
		}
	}
	if (!have_place) {	//找不到一个现有服务器有足够空间，重新买一个并装入虚拟机 
		buy_server_and_set_vm(v, id, server_and_node, daily_add);
	}
	vm_map[id] = v;
}

void buy_server_and_set_vm(Vm& v, int id, vector<vector<int>>& server_and_node, 
	unordered_map<string,vector<int>>& daily_add) {
//	购买服务器用最笨的方法，找到第一个符合条件的服务器 
	for (auto it = server_type_map.begin(); it != server_type_map.end(); ++it) {
		if (v.info.on_double) {	// 双节点
			if (it->second.cpu >= v.info.cpu && it->second.memory >= v.info.memory) {
				Server s;
				s.type = it->first;
				for (int i = 0; i < 2; ++i) {
					s.nodes.push_back(list<int>(1,id));
					s.remains.push_back(make_pair(it->second.cpu/2-v.info.cpu/2, it->second.memory/2-v.info.memory/2));
				}	
				//servers[server_index] = s;
				servers.push_back(s);
				v.server_id = server_index;
				server_and_node.push_back({server_index});
				daily_add[it->first].push_back(server_index);	// daily_add中，某型号的服务器，加入新购买订单的服务器ID
				server_index++;
				break;
			}
		}
		else {	// 单节点
			if (it->second.cpu/2 >= v.info.cpu && it->second.memory/2 >= v.info.memory) {
				Server s;
				s.type = it->first;
				s.nodes.resize(2);
				s.nodes[0].push_back(id);
				s.remains.resize(2);
				s.remains[0] = make_pair(it->second.cpu/2-v.info.cpu, it->second.memory/2-v.info.memory);
				s.remains[1] = make_pair(it->second.cpu/2, it->second.memory/2);
				//servers[server_index] = s;
				servers.push_back(s);
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
	Server& s = servers[v.server_id];
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

void migration(unordered_map<int, migration_order>& daily_migration, 
	vector<int>& new_vm, unordered_map<string, vector<int>>& daily_add, int vm_installed_cnt) {
// v1.0遍历虚拟机列表，寻找到创建顺序排名靠前的虚拟机来迁移
// 用贪心算法，从所有可选服务器中选择剩余空间最小的服务器(根据add中的搜索算法修改而来)
	int migration_cnt = vm_installed_cnt*5/1000;	// 确定可以迁移的次数
	int i=0;	// 目前迁移次数
	for (auto it = vm_map.begin(); it != vm_map.end(); ++it)
	{	// 遍历已有虚拟机列表
		if (i==migration_cnt)
			break;	// 处理完所有迁移请求后需要break退出循环
					// 或者是遍历完整个虚拟机列表后还未达到最大迁移次数，自动退出循环
		Vm &v = it->second;		// 选中需要迁移的虚拟机
		int v_ID = it->first;	// 选中需要迁移的虚拟机
		// 寻找位置并安置虚拟机
		bool have_place = false;
		if (v.info.on_double == 0) {	// 单节点VM
			int index, node, min_cpu_and_momory_remained = INT_MAX;	// 剩余空间最小服务器的信息
			for (int j = 0; j < servers.size(); ++j) {	// 搜索最佳服务器
				for (int i = 0; i < 2; ++i) {
					pair<int,int>& cpu_memory = servers[j].remains[i];
					if (cpu_memory.first >= v.info.cpu && cpu_memory.second >= v.info.memory
						&& cpu_memory.first + cpu_memory.second < min_cpu_and_momory_remained) {	// 更新最佳服务器的信息
							index = j;	// 目标服务器ID
							node = i;	// 目标节点
							have_place = true;
					}
				}
			}
			if (have_place) {	// 创建迁移申请
				// 原服务器删除目标虚拟机
				pair<int,int>& old_cpu_memory = servers[v.server_id].remains[v.node];	// 目标服务器剩余资源
				old_cpu_memory.first += v.info.cpu;		// 释放资源
				old_cpu_memory.second += v.info.memory;	// 释放资源
				servers[v.server_id].nodes[v.node].remove(v_ID);
				// 虚拟机装入目标服务器
				pair<int,int>& cpu_memory = servers[index].remains[node];
				servers[index].nodes[node].push_back(v_ID);
				cpu_memory.first -= v.info.cpu;
				cpu_memory.second -= v.info.memory;
				// 修改虚拟机信息
				v.server_id = index;
				v.node = node;
				// 如果发现是本日刚创建的虚拟机，则不需要加入daily_migration
				vector<int>::iterator it2;
				it2 = find(new_vm.begin(), new_vm.end(), v_ID);
				if (it2 != new_vm.end());
				else
				{
					// daily_migration加入新迁移申请以供输出
					daily_migration[it->first].target_ID = index;
					daily_migration[it->first].target_node = node;
					daily_migration[it->first].on_double = 0;
					i++;
				}
			}
		}
		else {	// 双节点VM
			int index, min_cpu_and_momory_remained = INT_MAX;	// 剩余空间最小服务器的信息
			for (int j = 0; j < servers.size(); ++j) {	// 搜索最佳服务器
				vector<pair<int,int>>& pairs = servers[j].remains;
				if (pairs[0].first >= v.info.cpu/2 && pairs[0].second >= v.info.memory/2
					&& pairs[1].first >= v.info.cpu/2 && pairs[1].second >= v.info.memory/2
					&& pairs[0].first + pairs[0].second + pairs[1].first + pairs[1].second < min_cpu_and_momory_remained ) {	// 更新最佳服务器的信息
						index = j;
						have_place = true;
					}
			}
			if (have_place) {	// 创建迁移申请
				// 原服务器删除目标虚拟机
				vector<pair<int,int>>& old_cpu_memory = servers[v.server_id].remains;
				for (int i = 0; i < 2; ++i) {	// 释放资源
					old_cpu_memory[i].first += v.info.cpu/2;
					old_cpu_memory[i].second += v.info.memory/2; 
					servers[v.server_id].nodes[i].remove(v_ID);
				}
				// 虚拟机装入目标服务器
				vector<pair<int,int>>& pairs = servers[index].remains;	// 目标服务器剩余资源
				for (int i = 0; i < 2; ++i) {
					servers[index].nodes[i].push_back(v_ID);
					pairs[i].first -= v.info.cpu/2;
					pairs[i].second -= v.info.memory/2; 
				}
				// 修改虚拟机信息
				v.server_id = index;
				// 如果发现是本日刚创建的虚拟机，则不需要加入daily_migration
				vector<int>::iterator it2;
				it2 = find(new_vm.begin(), new_vm.end(), v_ID);
				if (it2 != new_vm.end());
				else
				{
					// daily_migration加入新迁移申请以供输出
					daily_migration[it->first].target_ID = index;
					daily_migration[it->first].on_double = 1;
					i++;
				}
			}
		}
	}
/*
	// 如果迁移导致了少买服务器，则需要从daily_add中删除相应购买订单（未完成）（非必要）
	// 有逻辑错误！假设今天刚买的服务器A，上面装载的某虚拟机在当日里创建又删除，最后留下来的空服务器A也不能删除
	for (auto it1 = daily_add.begin(); it1 != daily_add.end(); ++it1)
	{	// 遍历所有本日买入的服务器
		// vector<int> new_servers = it1->second;	// it1->second存放本日购入的特定型号服务器的下标列表
		for (auto it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{	// 遍历所有每日购入的新服务器。it2类型是int，代表服务器下标
			if ((servers[*it2].nodes).empty())
			{	// 如果找到的新服务器上一台虚拟机都没有，那就删除这条扩容请求
				it1->second.erase(it2);	// 删除daily_add的相关记录
				// 删除servers相关记录
			}
		}
	}
*/
}

int get_vm_amount(Server server)
{
	set<int> vm_list;
	for (int i=0; i<2; i++)
	{
		for (auto it = server.nodes[i].begin(); it != server.nodes[i].end(); ++it)
		{
			vm_list.insert(*it);
		}
	}
	return vm_list.size();
}

