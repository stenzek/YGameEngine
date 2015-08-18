#pragma once
#include "Engine/Common.h"

class CommandQueue
{
public:
    // 1MiB default queue size
    static const uint32 DEFAULT_COMMAND_QUEUE_SIZE = 1048576;

private:
    struct CommandBase
    {
        virtual ~CommandBase() {}
        virtual void Execute() = 0;
    };

    template<class T>
    struct CommandLambdaTrampoline
    {
        T m_callback;

        CommandLambdaTrampoline(const T &callback) : m_callback(callback) {}
        CommandLambdaTrampoline(T &&callback) : m_callback(std::move(callback)) {}
        virtual ~CommandLambdaTrampoline() {}

        virtual void Execute() { m_callback(); }
    };

public:
    CommandQueue();
    ~CommandQueue();

    // Initialize the command queue
    bool Initialize(uint32 commandQueueSize = DEFAULT_COMMAND_QUEUE_SIZE, uint32 workerThreadCount = 1);

    // Initialize the command queue using a shared thread pool
    bool Initialize(ThreadPool *pThreadPool, uint32 commandQueueSize = DEFAULT_COMMAND_QUEUE_SIZE, bool yieldToOtherJobs = false);

    // Wait until all workers are finished, and then end the threads.
    void ExitWorkers();

    // Worker thread info
    const Thread *GetWorkerThread(uint32 idx) const { return m_workerThreads[idx]; }
    Thread::ThreadIdType GetWorkerThreadID(uint32 idx) const { return m_workerThreads[idx]->GetThreadId(); }
    uint32 GetWorkerThreadCount() const { return m_workerThreads.GetSize(); }

    // Pausing/resuming of thread
    void PauseWorkers();
    void ResumeWorkers();

    // Queuing of commands
    void QueueCommand(CommandBase *pCommand, uint32 commandSize);

    // Wrapper to assist with queueing lambda callbacks
    template<class T> void QueueLambdaCommand(const T &lambda)
    {
        if (m_commandQueueSize == 0)
        {
            lambda();
            return;
        }

        LockQueueForNewCommand();

        CommandLambdaTrampoline<T> *trampoline = (CommandLambdaTrampoline<T> *)FifoAllocateCommand(sizeof(CommandLambdaTrampoline<T>), false);
        new (trampoline)CommandLambdaTrampoline<T>(lambda);

        UnlockQueueForNewCommand();
    }

    // Queue lambda command using move semantics
    template<class T> void QueueLambdaCommand(T &&lambda)
    {
        if (m_commandQueueSize == 0)
        {
            lambda();
            return;
        }

        LockQueueForNewCommand();

        CommandLambdaTrampoline<T> *trampoline = (CommandLambdaTrampoline<T> *)FifoAllocateCommand(sizeof(CommandLambdaTrampoline<T>), false);
        new (trampoline) CommandLambdaTrampoline<T>(std::move(lambda));

        UnlockQueueForNewCommand();
    }

    // blocking variants
    void QueueBlockingCommand(CommandBase *pCommand, uint32 commandSize);
    template<class T> void QueueBlockingLambdaCommand(const T &lambda)
    {
        if (m_commandQueueSize == 0 || m_workerThreads.IsEmpty())
        {
            lambda();
            return;
        }

        // currently blocking events are only supported coming from the main thread
        DebugAssert(Thread::GetCurrentThreadId() == m_creatorThreadID);

        LockQueueForNewCommand();

        CommandLambdaTrampoline<T> *trampoline = (CommandLambdaTrampoline<T> *)FifoAllocateCommand(sizeof(CommandLambdaTrampoline<T>), true);
        new (trampoline)CommandLambdaTrampoline<T>(lambda);

        UnlockQueueForNewCommand();

        // block
        m_barrier.Wait();
    }

    // when not using a render thread, executes any pending commands, and blocks until the queue is empty
    // returns true if a command was executed, false if the queue was empty
    bool ExecuteQueuedCommands();

private:
    // worker thread class
    class WorkerThread : public Thread
    {
    public:
        WorkerThread(CommandQueue *pParent);

    protected:
        virtual int ThreadEntryPoint() override;
        CommandQueue *m_this;
    };

    // thread pool task class
    class ThreadPoolTask : public ThreadPoolWorkItem
    {
    public:
        ThreadPoolTask(CommandQueue *pParent);

        bool IsActive() const { return m_active; }
        void SetActive() { m_active = true; }

    protected:
        virtual int32 ProcessWork() override;
        CommandQueue *m_this;
        bool m_active;
    };

    // so the tasks can call our internal methods
    friend WorkerThread;
    friend ThreadPoolTask;

    // fifo queue entry
    struct FifoQueueEntryHeader
    {
        CommandBase *pCommand;
        uint32 Size;
        bool BlockingEvent;
    };

    // allocate the queue
    void AllocateQueue(uint32 size);
    
    // lock the queue
    void LockQueueForNewCommand();

    // unlock the queue, call this variant if no additional commands were added
    void UnlockQueueForNewCommand();

    // allocate bytes in the queue, assumes that the queue lock is held
    void *FifoAllocateCommand(uint32 size, bool blockingEvent);

    // fifo is empty?
    bool FifoIsEmpty() const;

    // retreives the first command off the fifo, but does not destroy it yet
    FifoQueueEntryHeader *FifoGetNextCommand();

    // release a fifo command
    void FifoReleaseCommand(FifoQueueEntryHeader *commandHdr);
     
    // creator thread
    Thread::ThreadIdType m_creatorThreadID;
    
    // worker thread
    PODArray<WorkerThread *> m_workerThreads;
    bool m_workerThreadExitFlag;

    // thread pool
    PODArray<ThreadPoolTask *> m_threadPoolTasks;
    ThreadPool *m_pThreadPool;
    volatile uint32 m_activeThreadPoolTasks;
    bool m_threadPoolYieldToOtherJobs;

    // fifo members
    PODArray<FifoQueueEntryHeader *> m_commandQueue;
    RecursiveMutex m_queueLock;
    volatile uint32 m_activeWorkerThreads;
    uint32 m_commandQueueSize;

    // events
    ConditionVariable m_conditionVariable;
    Barrier m_barrier;
};

