#pragma once
#include <list>
#include <vector>
#include <fstream>

struct Task
{
	size_t weight;
	size_t awaitTime;
	size_t timeLeft;
	size_t timeCreated;
	size_t timeFinished;
	size_t priority;
	Task(size_t weight = 0, size_t timeCreated = 0, size_t prior = 0)
	{
		this->weight = weight;
		this->timeCreated = timeCreated;
		priority = prior;
		awaitTime = 0;
		timeLeft = weight;
		timeFinished = 0;
	}
};

class TaskManager
{
public:
	TaskManager(size_t weightStart, size_t weightEnd, size_t intervalStart, size_t intervalEnd, size_t maxPrior);
	~TaskManager();
	void processTick();
	void saveData();
	void saveData2();
	void reset(size_t intervalEnd);
	size_t currentTick;
private:
	std::vector<Task> doneTasks;
	std::list<Task> queue;
	void addTask();
	void processTask();
	void updateAwaitingTime();
	void sortByPriority();
	size_t weightStart;
	size_t weightEnd;
	size_t intervalStart;
	size_t intervalEnd;
	size_t tickToAddTask;
	size_t taskAmount;
	size_t sleepAmount;
	size_t maxPriority;
	bool processingTask;
	Task currentTask;
	size_t timeToProcessTask;
	std::ofstream file;
	std::ofstream file2;
};

