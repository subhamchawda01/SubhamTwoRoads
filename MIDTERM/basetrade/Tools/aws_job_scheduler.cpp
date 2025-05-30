/**
   \file aws_job_scheduler.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <map>
#include <errno.h>
#include <sstream>

#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

/**
 *
 * The current class aims to implement a job scheduler
 * The class is not responsible for execution of a job
 *
 * we have a many jobs to execute which are divided in queues. each queue is assigned some attributes.
 * the scheduler aim to find the next job which must be schedule on any resource.
 * note that the scheduler does not aim to distinguish between different resources. it can be a work for future
 *
 * this class runs as a single threaded server which can be invoked by different agents to
 * accomplish job-assignment and job-queue management
 *
 */

#define AWS_JOB_Q_CONTROLLER_PORT 33533
class Queue;
class Job;

std::string q_dir;
std::string q_cfg;
HFSAT::BulkFileWriter q_log_writer;

class Queue {
 public:
  int qId;
  std::string name;
  int cores_consumed;
  int cores_allocated;
  int maxRunningChildren;
  int priority;
  int cooloff;
  int start_HHMM;
  int end_HHMM;
  bool enabled;

  Queue()
      : qId(-1),
        cores_consumed(0),
        cores_allocated(0),
        maxRunningChildren(0),
        priority(0),
        cooloff(86400),
        start_HHMM(0),
        end_HHMM(2400),
        enabled(true) {}

  static int UniqId() {
    static int id = 0;
    return ++id;
  }

  bool hasFreeCores() { return cores_consumed < cores_allocated; }

  double getScore() { return priority * 10 + (double)cores_consumed / std::max(1.000, (double)cores_allocated); }

  std::string toString() {
    static std::stringstream ss;
    ss.str("");
    ss << "id:" << qId << " name:" << name << " cores_consumed:" << cores_consumed
       << " cores_allocated:" << cores_allocated << " maxRunningChildren:" << maxRunningChildren
       << " priority:" << priority << " cooloff:" << cooloff << " start_HHMM:" << start_HHMM << " end_HHMM:" << end_HHMM
       << " enabled:" << enabled;

    return ss.str();
  }

  static void readFromFile(const std::string& file, std::map<std::string, int>& q_string,
                           std::map<int, Queue*>& qList) {
    HFSAT::BulkFileReader r;
    r.open(file);
    while (r.is_open()) {
      char line[2048];
      int readLen = r.GetLine(line, 2048);
      if (readLen > 0) {
        if (line[0] == '#') continue;

        std::vector<const char*> words = HFSAT::PerishableStringTokenizer(line, readLen).GetTokens();
        if (words.size() < 4) continue;

        Queue* q;
        if (q_string.find(words[0]) == q_string.end()) {
          int qId = Queue::UniqId();
          q_string[words[0]] = qId;
          q = new Queue();
          q->qId = qId;
          qList[qId] = q;
        } else {
          q = qList[q_string[words[0]]];
        }

        q->qId = q_string[words[0]];

        unsigned int indx = 0;
        q->name = words[indx];

        indx++;
        q->cores_allocated = atoi(words[indx]);

        indx++;
        q->priority = atoi(words[indx]);

        indx++;
        if (words.size() > indx) q->cooloff = atoi(words[indx]);

        indx++;
        if (words.size() > indx) q->start_HHMM = atoi(words[indx]);

        indx++;
        if (words.size() > indx) q->end_HHMM = atoi(words[indx]);

        indx++;
        if (words.size() > indx) q->maxRunningChildren = atoi(words[indx]);

        q->enabled = true;
      }
    }
  }
};

enum JobStatus { RESERVED, RUNNING, WAITING, COMPLETED };
class Job {
 public:
  Queue* queue;
  int jobId;
  std::string job;
  int lastRunStartTime;
  int lastRunEndTime;
  int currentStartTime;
  int numRuns;
  bool enabled;
  JobStatus jobStatus;
  int numRunningChildren;
  int aliveChildrenCount;
  Job* parent;
  char machineAssigned[64];

  Job()
      : queue(NULL),
        jobId(-1),
        job(""),
        lastRunStartTime(0),
        lastRunEndTime(0),
        currentStartTime(0),
        numRuns(0),
        enabled(true),
        jobStatus(COMPLETED),
        numRunningChildren(0),
        aliveChildrenCount(0),
        parent(NULL) {
    bzero(machineAssigned, 64);
  }

  Job(Job* parent_, const std::string& command2run)
      : queue(NULL),
        jobId(-1),
        job(command2run),
        lastRunStartTime(0),
        lastRunEndTime(0),
        currentStartTime(0),
        numRuns(0),
        enabled(true),
        jobStatus(COMPLETED),
        numRunningChildren(0),
        aliveChildrenCount(0),
        parent(parent_) {
    bzero(machineAssigned, 64);
    if (parent_ != NULL)
      queue = parent_->queue;
    else
      queue = NULL;
  }

  static int UniqId() {
    static int id = 0;
    return ++id;
  }

  bool isRunningOrWaiting() { return jobStatus == RUNNING || jobStatus == WAITING; }

  // This does not accumulate to top
  void addChildrenCount(int count) { aliveChildrenCount += count; }

  // this field is updated only for top level proc
  void addRunningChildren(int count) {
    if (parent == NULL) {
      numRunningChildren += count;
      return;
    }
    parent->addRunningChildren(count);
  }

  // This should be invoked for the parent job
  bool checkIfMoreSiblingsAllowed() {
    if (parent == NULL) {
      return numRunningChildren < queue->maxRunningChildren;
    }
    return parent->checkIfMoreSiblingsAllowed();
  }

  bool do_reserve(int timestamp) {
    if (jobStatus != COMPLETED) return false;
    jobStatus = RESERVED;
    queue->cores_consumed++;
    if (parent != NULL) parent->addRunningChildren(1);
    currentStartTime = timestamp;
    return true;
  }

  bool undo_reserve() {
    if (jobStatus != RESERVED) return false;
    jobStatus = COMPLETED;
    queue->cores_consumed--;
    if (parent != NULL) parent->addRunningChildren(-1);
    currentStartTime = 0;
    return true;
  }

  bool setRunningStatus(const std::string& machine) {
    if (jobStatus == RESERVED) {
      jobStatus = RUNNING;
      timeval tv;
      gettimeofday(&tv, NULL);
      currentStartTime = tv.tv_sec;
      // queue->cores_consumed++; //already done while reserving the job
      unsigned long max_length = 64;
      strncpy(machineAssigned, machine.c_str(), std::min(max_length, machine.length()));
      //        if ( parent!= NULL)
      //          parent->addRunningChildren(1);
      return true;
    } else {
      return false;
    }
  }

  // will be called whenever is called for self or any child processes
  bool setCompletedStatus(bool calledForSelf) {
    if (isRunningOrWaiting()) {
      if (aliveChildrenCount > 0) {
        if (calledForSelf) jobStatus = WAITING;  // cant be completed if we have alivechildren
        return false;
      }
      jobStatus = COMPLETED;
      if (parent == NULL) {
        // top level proc, update relevant structures
        numRuns++;
        timeval tv;
        gettimeofday(&tv, NULL);
        lastRunEndTime = tv.tv_sec;
        lastRunStartTime = currentStartTime;
        currentStartTime = 0;
      } else {
        parent->addChildrenCount(-1);
      }
      return true;
    } else {
      return false;
    }
  }

  std::string toString() {
    static std::stringstream ss;
    ss.str("");
    ss << "qId:" << queue->qId << " jobId:" << jobId << " job:" << job << " lastRunStartTime:" << lastRunStartTime
       << " lastRunEndTime:" << lastRunEndTime << " currentStartTime:" << currentStartTime << " numRuns:" << numRuns
       << " enabled:" << enabled << " status:";
    switch (jobStatus) {
      case RESERVED:
        ss << "reserved";
        break;
      case RUNNING:
        ss << "running";
        break;
      case WAITING:
        ss << "waiting";
        break;
      case COMPLETED:
        ss << "completed";
        break;
      default:
        ss << "undefined_status";
        break;
    }
    ss << " numRunningChildren:" << numRunningChildren << " aliveChildrenCount:" << aliveChildrenCount
       << " parentId:" << (parent == NULL ? -1 : parent->jobId) << " machineAssigned:" << machineAssigned;
    return ss.str();
  }

  static std::string readFromFile(const std::string& file, std::map<std::string, int>& q_string,
                                  std::map<int, Queue*>& qList, std::map<std::string, int>& job_string,
                                  std::map<int, Job*>& jobList) {
    char response[1024];
    if (q_string.find(file) == q_string.end()) {
      sprintf(response, "q %s not in q.config", file.c_str());
      return response;
    }
    int qId = q_string[file];

    HFSAT::BulkFileReader r;
    r.open(q_dir + file);
    if (!r.is_open()) {
      sprintf(response, "file can't be read %s/%s", q_dir.c_str(), file.c_str());
      return response;
    }

    int num_jobs_added = 0;
    int num_dups = 0;
    while (r.is_open()) {
      char command[2048];
      int readLen = r.GetLine(command, 2048);
      if (readLen > 1) {  // 1 because we don't want simply a new line (\n)
        if (command[0] == '#') continue;
        command[--readLen] = '\0';
        if (job_string.find(command) == job_string.end()) {
          int jobId = Job::UniqId();
          job_string[command] = jobId;
          Job* j = new Job();
          j->queue = qList[qId];
          j->job = command;
          j->jobId = job_string[command];
          j->numRuns = 0;
          j->lastRunStartTime = 0;
          jobList[jobId] = j;
          num_jobs_added++;
        } else {
          jobList[job_string[command]]->enabled = true;
          num_dups++;
        }
      }
    }
    sprintf(response, "num jobs added %d num-duplicates %d", num_jobs_added, num_dups);
    return response;
  }

  bool checkCoolOffAndTimeOfDay(int current_time_in_secs) {
    int secs_since_midnight = current_time_in_secs % 86400;
    int current_hhmm = (secs_since_midnight / 3600) * 100 + (secs_since_midnight % 3600) / 60;

    // last run completion too recent, don't schedule now
    if ((lastRunEndTime + queue->cooloff) > current_time_in_secs) return false;
    if (current_hhmm < queue->start_HHMM || current_hhmm > queue->end_HHMM) return false;
    return true;
  }

  double getScore(int current_time_in_secs) {
    double starvation_factor = (current_time_in_secs - lastRunEndTime) / std::max(1.0, (double)queue->cooloff);
    starvation_factor = std::min(
        starvation_factor, 10.0);  // now its value is 1 to 10. higher value means we should run this more frequently
    return queue->getScore() * 100 + 10 / starvation_factor;  // low score get priority
  }
};

class Scheduler {
  std::vector<std::string> commands;
  enum command {
    help,
    job_reserve,
    job_unreserve,
    job_begin,
    job_fork_child,
    job_end,

    view_jobs_all,
    view_jobs,
    view_jobs_running,
    view_jobs_on_machine,
    view_jobs_for_queue,
    view_queues_all,

    enable_queue,
    disable_queue,
    enable_job,
    disable_job,

    reload_all,
    reload_config,
    reload_queue,
    undefined
  };

  command getCommandFromString(std::string str) {
    if (str.compare("help") == 0) return help;
    if (str.compare("job_reserve") == 0) return job_reserve;
    if (str.compare("job_unreserve") == 0) return job_unreserve;
    if (str.compare("job_begin") == 0) return job_begin;
    if (str.compare("job_fork_child") == 0) return job_fork_child;
    if (str.compare("job_end") == 0) return job_end;

    if (str.compare("view_jobs_all") == 0) return view_jobs_all;
    if (str.compare("view_jobs") == 0) return view_jobs;
    if (str.compare("view_jobs_running") == 0) return view_jobs_running;
    if (str.compare("view_jobs_on_machine") == 0) return view_jobs_on_machine;
    if (str.compare("view_jobs_for_queue") == 0) return view_jobs_for_queue;
    if (str.compare("view_queues_all") == 0) return view_queues_all;

    if (str.compare("enable_queue") == 0) return enable_queue;
    if (str.compare("disable_queue") == 0) return disable_queue;
    if (str.compare("enable_job") == 0) return enable_job;
    if (str.compare("disable_job") == 0) return disable_job;

    if (str.compare("reload_all") == 0) return reload_all;
    if (str.compare("reload_config") == 0) return reload_config;
    if (str.compare("reload_queue") == 0) return reload_queue;
    return undefined;
  }

  std::string stringFromCommand(command c) {
    switch (c) {
      case help:
        return "help";

      case job_reserve:
        return "job_reserve";
      case job_unreserve:
        return "job_unreserve";
      case job_begin:
        return "job_begin";
      case job_fork_child:
        return "job_fork_child";
      case job_end:
        return "job_end";

      case view_jobs_all:
        return "view_jobs_all";
      case view_jobs:
        return "view_jobs";
      case view_jobs_running:
        return "view_jobs_running";
      case view_jobs_on_machine:
        return "view_jobs_on_machine";
      case view_jobs_for_queue:
        return "view_jobs_for_queue";
      case view_queues_all:
        return "view_queues_all";

      case enable_queue:
        return "enable_queue";
      case disable_queue:
        return "disable_queue";
      case enable_job:
        return "enable_job";
      case disable_job:
        return "disable_job";

      case reload_all:
        return "reload_all";
      case reload_config:
        return "reload_config";
      case reload_queue:
        return "reload_queue";
      default:
        return "undefined";
    }
  }

  std::map<int, Queue*> qList;
  std::map<int, Job*> jobList;
  std::vector<Job*> childJobs;

  std::map<std::string, int> q_string;
  std::map<std::string, int> job_string;

 public:
  Scheduler() : commands(), qList(), jobList(), q_string(), job_string() {
    commands.push_back("help");

    commands.push_back("job_reserve");
    commands.push_back("job_unreserve int:jobId");
    commands.push_back("job_begin int:jobId int:machineId");
    commands.push_back("job_fork_child int:parentJobId multi_word:command");
    commands.push_back("job_end int:jobId\n");

    commands.push_back("view_jobs_all");
    commands.push_back("view_jobs int:jobId1 int:jobId2 ... int:jobIdn");
    commands.push_back("view_jobs_running");
    commands.push_back("view_jobs_on_machine word:machine_id");
    commands.push_back("view_jobs_for_queue int:qId OR view_jobs_for_queue word:q_name");
    commands.push_back("view_queues_all\n");

    commands.push_back("enable_queue int:queueId");
    commands.push_back("disable_queue int:queueId");
    commands.push_back("enable_job int:jobId");
    commands.push_back("disable_job int:jobId\n");

    commands.push_back("reload_all");
    commands.push_back("reload_config");
    commands.push_back("reload_queue word:queue_name");
  }

  // finds the job that is best suited for allocation
  // highest priority to child procs
  // next sort on queue priority
  // next sort on job-waiting-time/cooloff-period
  std::string fetchJob() {
#define SECS_TO_WAIT_FOR_CONFIRMATION 120
    timeval tv;
    gettimeofday(&tv, NULL);

    int best_index = -1;
    double best_score = 1e9;
    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      // if job is in RESERVED status for long, revert its status to not_running
      if (it->second->jobStatus == RESERVED &&
          SECS_TO_WAIT_FOR_CONFIRMATION < (tv.tv_sec - it->second->currentStartTime)) {
        it->second->jobStatus = COMPLETED;
        it->second->queue->cores_consumed--;
      }

      // job or queue is disabled, do not consider this job for allocation
      if (it->second->queue->enabled == false || it->second->enabled == false ||
          it->second->checkCoolOffAndTimeOfDay(tv.tv_sec) == false || it->second->queue->hasFreeCores() == false)
        continue;

      double score = 0;

      // check if this job is a child job and is eligible, decrease its score by a large value to ensure priority
      if (it->second->parent != NULL) {
        if (false == it->second->checkIfMoreSiblingsAllowed()) {
          continue;
        }
        score -= 100000;
      }

      // consider jobs with status as not running for scoring and hence allocation
      if (it->second->jobStatus == COMPLETED) {
        score += it->second->getScore(tv.tv_sec);
        if (score < best_score) {
          best_index = it->second->jobId;
          best_score = score;
        }
      }
    }

    if (best_index != -1) {
      jobList[best_index]->do_reserve(tv.tv_sec);
      static std::stringstream ss;
      ss.str("");
      ss << "jobId:" << best_index << " " << jobList[best_index]->job;
      return ss.str();
    }
    return "jobId:-1 no_job";
  }

  std::string getAllQueues() {
    static std::stringstream ss;
    ss.str("");
    for (std::map<int, Queue*>::iterator it = qList.begin(); it != qList.end(); ++it) {
      ss << it->second->toString() << "\n";
    }
    return ss.str();
  }

  std::string getAllJobs() {
    static std::stringstream ss;
    ss.str("");

    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      ss << it->second->toString() << "\n";
    }
    return ss.str();
  }

  std::string getAllJobsOnQueueId(int qId) {
    static std::stringstream ss;
    ss.str("");

    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      if (it->second->queue->qId != qId) continue;
      ss << it->second->toString() << "\n";
    }
    return ss.str();
  }

  std::string getRunningJobs() {
    static std::stringstream ss;
    ss.str("");

    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      if (it->second->isRunningOrWaiting()) ss << it->second->toString() << "\n";
    }
    return ss.str();
  }

  std::string getRunningJobsOnMachine(const std::string& machine) {
    static std::stringstream ss;
    ss.str("");

    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      if (it->second->isRunningOrWaiting() && strcmp(it->second->machineAssigned, machine.c_str()) == 0)
        ss << it->second->toString() << "\n";
    }
    return ss.str();
  }

  std::string getHelp() {
    static std::stringstream ss;
    ss.str("");
    for (auto i = 0u; i < commands.size(); ++i) {
      ss << commands[i] << "\n";
    }
    return ss.str();
  }

  void reloadQueue(int qId) {
    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end(); ++it) {
      if (it->second->queue->qId == qId) it->second->enabled = false;
    }
    std::string retval = Job::readFromFile(qList[qId]->name, q_string, qList, job_string, jobList);
    for (std::map<int, Queue*>::iterator it = qList.begin(); it != qList.end(); ++it) {
      eraseDisabledJobs(it->second->qId, false);
    }
  }

  std::string reloadQueue(const std::string& qName) {
    if (q_string.find(qName) == q_string.end()) {
      return "invalid_q_name " + qName;
    }
    int qId = q_string[qName];
    reloadQueue(qId);
    return "reload_queue called for " + qName;
  }

  void eraseDisabledJobs(int qId, bool ignoreStatus) {
    for (std::map<int, Job*>::iterator it = jobList.begin(); it != jobList.end();) {
      if (qId == it->second->queue->qId && (ignoreStatus || it->second->enabled == false)) {
        job_string.erase(it->second->job);
        Job* j = it->second;
        it = jobList.erase(it);
        delete j;
        j = NULL;
      } else {
        ++it;
      }
    }
  }

  void reloadCfg() {
    for (std::map<int, Queue*>::iterator it = qList.begin(); it != qList.end(); ++it) {
      it->second->enabled = false;
    }
    Queue::readFromFile(q_cfg, q_string, qList);
    // delete queues which are disabled after deleting
    for (std::map<int, Queue*>::iterator it = qList.begin(); it != qList.end();) {
      if (it->second->enabled == false) {
        q_string.erase(it->second->name);
        Queue* q = it->second;
        eraseDisabledJobs(q->qId, true);
        it = qList.erase(it);
        delete q;
        q = NULL;
      } else {
        ++it;
      }
    }
  }

  std::string reloadAll() {
    static std::stringstream ss;
    ss.str("");
    reloadCfg();
    for (std::map<int, Queue*>::iterator it = qList.begin(); it != qList.end(); ++it) {
      reloadQueue(it->second->qId);
      ss << "load queue file " << it->second->name << "\n";
    }
    return ss.str();
  }

  std::string checkQId(int qId, bool& success) {
    success = false;
    if (qList.find(qId) == qList.end()) {
      char response[1024];
      sprintf(response, "incorrect qId %d", qId);
      return response;
    }
    success = true;
    return "";
  }

  std::string checkJobId(int jobId, bool& success) {
    success = false;
    if (jobList.find(jobId) == jobList.end()) {
      char response[1024];
      sprintf(response, "incorrect jobId %d", jobId);
      return response;
    }
    success = true;
    return "";
  }

  void addChildTask(int parentJobId, const std::string& command) {
    Job* j = new Job(jobList[parentJobId], command);
    j->jobId = Job::UniqId();
    jobList[j->jobId] = j;
    j->parent->aliveChildrenCount++;
  }

  std::string processCommands(const std::vector<const char*>& words) {
    if (words.size() < 1) return "no command/empty message. send help to see list of commands";
    command comm = getCommandFromString(words[0]);
    switch (comm) {
      case help:
        return getHelp();
      case job_reserve:
        return fetchJob();
      case job_unreserve: {
        if (words.size() < 2u) {
          return "incorrect syntax: job_unreserve <jobId>";
        }
        int jobId = atoi(words[1]);
        bool success;
        std::string resp = checkJobId(jobId, success);
        if (!success) {
          return resp;
        } else {
          // we expect child procs to be many. we don't ensure uniqueness in command. hence we have a process to clean
          // them up
          // if we find it eligible for deletion
          if (jobList[jobId]->undo_reserve()) {
            return "success";
          } else {
            return "job not in RESERVED status. cant execute command.";
          }
        }
      } break;
      case job_begin: {
        if (words.size() < 3u) {
          return "incorrect syntax: job_begin <jobId> <machineId>";
        }
        int jobId = atoi(words[1]);
        bool success;
        std::string resp = checkJobId(jobId, success);
        if (!success) {
          return resp;
        } else {
          return (jobList[jobId]->setRunningStatus(words[2]) ? "success"
                                                             : "unexpectedJobStatus. not marked as running");
        }
      } break;
      case job_fork_child: {
        if (words.size() < 3u) {
          return "incorrect syntax: job_fork_child <parentJobId> <command(multi_word)>";
        }
        int parentJobId = atoi(words[1]);
        bool success;
        std::string resp = checkJobId(parentJobId, success);
        if (!success) {
          return resp;
        } else {
          Job* parent = jobList[parentJobId];
          if (!parent->isRunningOrWaiting()) {  // Ideally only RUNNING should be allowed, but we don't care
            return "parentJob not running. can't spawn children";
          }
          static std::stringstream ss;
          ss.str("");
          ss << words[2];
          for (unsigned int i = 3; i < words.size(); ++i) {
            ss << " " << words[i];
          }
          addChildTask(parentJobId, ss.str());
          return "successfully added";
        }
      } break;
      case job_end: {
        if (words.size() < 2u) {
          return "incorrect syntax: job_end <jobId>";
        }
        int jobId = atoi(words[1]);
        bool success;
        std::string resp = checkJobId(jobId, success);
        if (!success) {
          return resp;
        } else {
          // we expect child procs to be many. we don't ensure uniqueness in command. hence we have a process to clean
          // them up
          // if we find it eligible for deletion
          Job* j = jobList[jobId];
          if (j->jobStatus == RUNNING) {
            if (j->parent != NULL)
              j->addRunningChildren(-1);  // only updates toplevel parent. top level parent does not call it for self
            j->queue->cores_consumed--;   // should be called just once

            bool calledForSelf = true;
            // now we have an iterative way to remove children who can be removed
            while (j != NULL && j->setCompletedStatus(calledForSelf) == true) {
              calledForSelf = false;
              Job* p = j->parent;
              if (p != NULL) {
                jobList.erase(j->jobId);
                delete j;
              }
              j = p;
            }
            return "success";
          }
          return "job is not running. can't mark its end";
        }
      } break;
      case view_jobs_all:
        return getAllJobs();
      case view_jobs: {
        static std::stringstream ss;
        ss.str("");
        for (unsigned int i = 1; i < words.size(); ++i) {
          int jobId = atoi(words[i]);
          std::map<int, Job*>::iterator it = jobList.find(jobId);
          if (it == jobList.end())
            ss << "incorrect jobId " << words[i] << "\n";
          else
            ss << it->second->toString() << "\n";
        }
        return ss.str();
      } break;
      case view_jobs_running:
        return getRunningJobs();
      case view_jobs_on_machine:
        if (words.size() < 2u) {
          return "incorrect syntax: view_jobs_on_machine <machineId>";
        }
        return getRunningJobsOnMachine(words[1]);
      case view_jobs_for_queue: {
        if (words.size() < 2u) {
          return "incorrect syntax: view_jobs_for_queue <qId/qName>";
        }
        if (q_string.find(words[1]) == q_string.end()) {
          int qId = atoi(words[1]);
          if (qList.find(qId) == qList.end()) {
            return "q_name or q_id not found";
          }
          return getAllJobsOnQueueId(qId);
        }
        return getAllJobsOnQueueId(q_string[words[1]]);
      } break;
      case view_queues_all:
        return getAllQueues();
      case enable_queue: {
        if (words.size() < 2u) {
          return "incorrect syntax: enable_queue <qId>";
        }
        int qId = atoi(words[1]);
        bool success = false;
        std::string resp = checkQId(qId, success);
        if (!success) {
          return resp;
        } else {
          qList[qId]->enabled = true;
          return "enable_queue successful";
        }
      } break;
      case disable_queue: {
        if (words.size() < 2u) {
          return "incorrect syntax: disable_queue <qId>";
        }
        int qId = atoi(words[1]);
        bool success = false;
        std::string resp = checkQId(qId, success);
        if (!success) {
          return resp;
        } else {
          qList[qId]->enabled = false;
          return "disable_queue successful";
        }
      } break;
      case enable_job: {
        if (words.size() < 2u) {
          return "incorrect syntax: enable_job <qId>";
        }
        int jobId = atoi(words[1]);
        bool success = false;
        std::string resp = checkJobId(jobId, success);
        if (!success) {
          return resp;
        } else {
          jobList[jobId]->enabled = true;
          return "enable_job successful";
        }
      } break;
      case disable_job: {
        if (words.size() < 2u) {
          return "incorrect syntax: disable_job <jobId>";
        }
        int jobId = atoi(words[1]);
        bool success = false;
        std::string resp = checkJobId(jobId, success);
        if (!success) {
          return resp;
        } else {
          jobList[jobId]->enabled = false;
          return "disable_job successful";
        }
      } break;
      case reload_all: {
        return reloadAll();
      } break;
      case reload_config:
        reloadCfg();
        return "reloaded q_config only. jobs for new queue(s) not loaded. jobs from deleted queue(s) also deleted.";
      case reload_queue: {
        if (words.size() < 2u) {
          return "incorrect syntax: enable_queue <q_name>";
        }
        return reloadQueue(words[1]);
      } break;
      default:
        return "undefined command. send help to get a list of commands";
    }
    return "";
  }
};

class SchedularServer {
 public:
  SchedularServer() : schedular() {}

  void init() {
    std::cerr << "Setting up TCP Server Socket on port " << AWS_JOB_Q_CONTROLLER_PORT << "\n";
    schedular.reloadAll();
    tcp_server_socket_ = new HFSAT::TCPServerSocket(AWS_JOB_Q_CONTROLLER_PORT);
  }

  void stop() {
    if (tcp_server_socket_->IsOpen()) {
      tcp_server_socket_->Close();
    }
  }

  ~SchedularServer() { stop(); }

  /*  Read line from a socket  */
  size_t Readline(int sockd, char* vptr, size_t maxlen) {
    size_t n, rc;
    char c, *buffer;

    buffer = vptr;

    for (n = 1; n < maxlen; n++) {
      if ((rc = read(sockd, &c, 1)) == 1) {
        *buffer++ = c;
        if (c == '\n') break;
      } else if (rc == 0) {
        if (n == 1)
          return 0;
        else
          break;
      } else {
        if (errno == EINTR) continue;
        return -1;
      }
    }

    *buffer = 0;
    return n;
  }

  /*  Write a line to a socket  */
  size_t Writeline(int sockd, const char* vptr, size_t n) {
    size_t nleft;
    size_t nwritten;
    const char* buffer;

    buffer = vptr;
    nleft = n;

    while (nleft > 0) {
      if ((nwritten = write(sockd, buffer, nleft)) <= 0) {
        if (errno == EINTR)
          nwritten = 0;
        else
          return -1;
      }
      nleft -= nwritten;
      buffer += nwritten;
    }

    return n;
  }

  void run() {
#define COMMAND_BUFFER_LEN 1024
    char command_buffer_[COMMAND_BUFFER_LEN];

    // select related vars
    fd_set rfd;

    while (int connected_socket_file_descriptor_ = tcp_server_socket_->Accept()) {
      int maxfdplus1 = connected_socket_file_descriptor_ + 1;
      bzero(command_buffer_, COMMAND_BUFFER_LEN);
      FD_ZERO(&rfd);
      FD_SET(maxfdplus1 - 1, &rfd);

      struct timespec timeout_pselect_;
      timeout_pselect_.tv_sec = 2;
      timeout_pselect_.tv_nsec = 0;
      int retval = pselect(maxfdplus1, &rfd, NULL, NULL, &timeout_pselect_, NULL);
      if (retval == 0) {
        close(connected_socket_file_descriptor_);
        continue;
      }
      if (retval == -1) {
        close(connected_socket_file_descriptor_);
        std::cerr << " Select returned -1 .\n ";
      }

      int read_len_ = Readline(connected_socket_file_descriptor_, command_buffer_, COMMAND_BUFFER_LEN);

      if (read_len_ <= 0) {
        close(connected_socket_file_descriptor_);
        continue;
      }

      std::vector<const char*> words = HFSAT::PerishableStringTokenizer(command_buffer_, read_len_).GetTokens();
      std::string response = schedular.processCommands(words);
      std::cerr << "sending response:: " << response << "\n";
      Writeline(connected_socket_file_descriptor_, response.c_str(), response.size());
      close(connected_socket_file_descriptor_);
    }
  }

 protected:
  HFSAT::TCPServerSocket* tcp_server_socket_;
  Scheduler schedular;
};

SchedularServer* schedulerServer;

static struct option data_options[] = {{"cfg", required_argument, 0, 'c'},
                                       {"qdir", required_argument, 0, 'q'},
                                       {"help", no_argument, 0, 'h'},
                                       {0, 0, 0, 0}};

/// signal handler
void sighandler(int signum) {
  fprintf(stderr, "Received signal %d \n", signum);
  if (schedulerServer != NULL) {
    schedulerServer->stop();
    delete schedulerServer;
    schedulerServer = NULL;
  }

  exit(0);
}

int main(int argc, char** argv) {
  int c;
  int hflag = 0;
  q_dir = "";
  q_cfg = "";

  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", data_options, &option_index);
    if (c == -1) break;
    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case 'q':
        q_dir = optarg;
        break;

      case 'c':
        q_cfg = optarg;
        break;

      case '?':
        if (optopt == 'q' || optopt == 'c') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }
  if (q_dir.empty() || q_cfg.empty()) hflag = 1;

  if (hflag) {
    std::cerr << "usage: " << argv[0] << " --qdir <q_directory> --cfg <q_config_file>\n";
    exit(-1);
  }

  /// set signal handler .. add other signals later
  struct sigaction sigact;
  sigact.sa_handler = sighandler;
  sigaction(SIGINT, &sigact, NULL);

  schedulerServer = new SchedularServer();

  schedulerServer->init();

  schedulerServer->run();
}
