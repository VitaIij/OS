#include <ctime>
#include "TaskManager.h"
#include <iostream>

using namespace std;

TaskManager::TaskManager(size_t weightStart, size_t weightEnd, size_t intervalStart, size_t intervalEnd, size_t maxPrior)
{
	tickToAddTask = currentTick = 1;
	this->weightStart = weightStart;
	this->weightEnd = weightEnd;
	this->intervalStart = intervalStart;
	this->intervalEnd = intervalEnd;
	maxPriority = maxPrior;
	processingTask = false;
	taskAmount = sleepAmount = 0;
	queue = list<Task>();
	srand(time(0));
	file.open("data.csv", ios::out | ios::trunc);
	file2.open("taskAwaiting.csv", ios::out | ios::trunc);
}

TaskManager::~TaskManager()
{
	file.close();
	file2.close();
}

void TaskManager::reset(size_t interval)
{
	this->intervalEnd = this->intervalStart = interval;
	tickToAddTask = currentTick = 1;
	processingTask = false;
	taskAmount = sleepAmount = 0;
	queue = list<Task>();
	doneTasks = vector<Task>();
}

void TaskManager::processTick()
{
	if (tickToAddTask == currentTick)
	{
		addTask();
		tickToAddTask = currentTick + rand() % (intervalEnd - intervalStart + 1) + intervalStart;
	}

	if (processingTask)
	{
		processTask();
	}
	else
	{
		timeToProcessTask = -1;

		if (queue.size())
		{
			currentTask = queue.front();
			queue.pop_front();

			timeToProcessTask = 1;
		}

		if (timeToProcessTask != -1)
		{
			processingTask = true;
			processTask();
		}
		else
		{
			sleepAmount++;
		}
	}
	
	currentTick++;
}

void TaskManager::addTask()
{
	size_t weight = rand() % (weightEnd - weightStart + 1) + weightStart;
	size_t priority = rand() % (maxPriority + 1);
	Task task(weight, currentTick, priority);
	queue.push_back(task);
	taskAmount++;
}

void TaskManager::processTask()
{
	currentTask.timeLeft--;
	timeToProcessTask--;
	updateAwaitingTime();
	if (!currentTask.timeLeft)
	{
		timeToProcessTask = 0;
		processingTask = false;
		currentTask.timeFinished = currentTick;
		doneTasks.push_back(currentTask);
		taskAmount--;
	}
	else if (!timeToProcessTask)
	{
		processingTask = false;
		queue.push_back(currentTask);
		sortByPriority();
	}
}

void TaskManager::sortByPriority()
{
	for (int j = maxPriority; j >= 0; j--)
	{
		for (auto i = queue.begin(), end = queue.end(); i != end; i++)
		{
			if ((*i).priority == j)
			{
				Task task = *i;
				queue.erase(i);
				queue.push_front(task);
				return;
			}
		}
	}
	//queue.sort([](const Task& a, const Task& b) { return a.priority > b.priority; });
}

void TaskManager::updateAwaitingTime()
{
	for (Task &task : queue)
	{
		task.awaitTime++;
	}
}

void TaskManager::saveData()
{
	double avAwTime = 0;
	for (size_t i = 0; i < doneTasks.size(); i++)
	{
		avAwTime += doneTasks[i].awaitTime;
	}
	for (Task task : queue)
	{
		avAwTime += task.awaitTime;
	}

	avAwTime = avAwTime / (taskAmount + doneTasks.size());
	file << intervalStart << ';' << (long)avAwTime << ';' << (double)sleepAmount / currentTick * 100 << endl;
}

void TaskManager::saveData2()
{
	for (size_t i = 0; i <= maxPriority; i++)
	{
		double avAwTime = 0;
		int couter = 0;
		for (size_t	j = 0; j < doneTasks.size(); j++)
		{
			if (doneTasks[j].priority == i)
			{
				avAwTime += doneTasks[i].awaitTime;
				couter++;
			}
		}
		for (Task task : queue)
		{
			if (task.priority == i)
			{
				avAwTime += task.awaitTime;
				couter++;
			}
		}

		avAwTime = avAwTime / couter;
		file2 << i << ';' << (long)avAwTime << endl;
	}
}
