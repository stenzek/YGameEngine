#include "Engine/PrecompiledHeader.h"
#include "Engine/CommandQueue.h"

CommandQueue::CommandQueue()
    : m_creatorThreadID(0),
      m_workerThreadExitFlag(true),
      m_activeThreadPoolTasks(0),
      m_threadPoolYieldToOtherJobs(false),
      m_activeWorkerThreads(0),
      m_commandQueueSize(0),
      m_barrier(2)
{

}

CommandQueue::~CommandQueue()
{
    // cleanup thread pool tasks
    if (m_pThreadPool != nullptr)
    {
        for (;;)
        {
            m_queueLock.Lock();
            if (m_activeThreadPoolTasks == 0)
                break;

            // loop until empty
            m_queueLock.Unlock();
            Thread::Yield();
            continue;
        }

        // tasks are now done, so kill off the references
        while (m_threadPoolTasks.GetSize() > 0)
        {
            ThreadPoolTask *pTask = m_threadPoolTasks.PopBack();
            pTask->Release();
        }

        // release lock
        m_queueLock.Unlock();
    }

    // cleanup queue and end threads
    ExitWorkers();

    // the queue should be empty at this point
    DebugAssert(FifoIsEmpty());
}

bool CommandQueue::Initialize(uint32 commandQueueSize /* = DEFAULT_COMMAND_QUEUE_SIZE */, uint32 workerThreadCount /* = 1 */)
{
    DebugAssert(m_workerThreadExitFlag && m_workerThreads.IsEmpty() && m_pThreadPool == nullptr);
    m_creatorThreadID = Thread::GetCurrentThreadId();

    // allocate queue
    AllocateQueue(commandQueueSize);

    // create worker threads
    if (workerThreadCount > 0)
    {
        // prevent threads exiting
        m_workerThreadExitFlag = false;
        MemoryBarrier();

        // create the workers
        for (uint32 i = 0; i < workerThreadCount; i++)
        {
            WorkerThread *pThread = new WorkerThread(this);
            if (!pThread->Start())
            {
                // cleanup the remaining threads
                delete pThread;
                ExitWorkers();
                return false;
            }

            // store thread
            m_workerThreads.Add(pThread);
        }
    }

    return true;
}

bool CommandQueue::Initialize(ThreadPool *pThreadPool, uint32 commandQueueSize /* = DEFAULT_COMMAND_QUEUE_SIZE */, bool yieldToOtherJobs /* = false */)
{
    DebugAssert(m_workerThreadExitFlag && m_workerThreads.IsEmpty() && m_pThreadPool == nullptr);
    m_creatorThreadID = Thread::GetCurrentThreadId();
    m_pThreadPool = pThreadPool;

    // allocate queue
    AllocateQueue(commandQueueSize);

    // allocate thread pool tasks, with a kept reference so that we can re-use them.
    uint32 taskCount = m_pThreadPool->GetWorkerThreadCount();
    m_threadPoolTasks.Resize(taskCount);
    for (uint32 i = 0; i < taskCount; i++)
        m_threadPoolTasks[i] = new ThreadPoolTask(this);

    // all done for now
    m_threadPoolYieldToOtherJobs = yieldToOtherJobs;
    return true;
}

void CommandQueue::AllocateQueue(uint32 size)
{
    m_commandQueueSize = size;
    if (size > 0)
    {
        // todo: allocate fifo
    }
}

void CommandQueue::ExitWorkers()
{
    // if there's no workers, just drain the queue
    if (m_workerThreads.IsEmpty())
    {
        ExecuteQueuedCommands();
        return;
    }

    // pause the workers, draining the queue
    PauseWorkers();

    // set the worker exit flag, and wake all workers
    m_workerThreadExitFlag = true;
    m_conditionVariable.WakeAll();
    ResumeWorkers();

    // join each thread
    while (m_workerThreads.GetSize() > 0)
    {
        WorkerThread *pThread = m_workerThreads.PopBack();
        pThread->Join();
        delete pThread;
    }
}

void CommandQueue::PauseWorkers()
{
    if (m_workerThreads.IsEmpty())
    {
        ExecuteQueuedCommands();
        return;
    }

    // drain the queue, then pause the thread by holding the lock
    for (;;)
    {
        m_queueLock.Lock();

        // queue has entries? wait for the worker to sleep
        if (!FifoIsEmpty() || m_activeWorkerThreads > 0)
        {
            if (m_activeWorkerThreads == 0)
            {
                m_conditionVariable.Wake();
                //Log::GetInstance().Write("w", LOGLEVEL_DEV, "Woke CV");
            }
            m_queueLock.Unlock();
            Thread::Yield();
            continue;
        }
        else
        {
            // leave the queue locked
            break;
        }
    }
}

void CommandQueue::ResumeWorkers()
{
    if (m_workerThreads.IsEmpty())
        return;

    // unlock queue, any workers that were woken should be allowed to continue
    m_queueLock.Unlock();
}

void CommandQueue::QueueCommand(CommandBase *pCommand, uint32 commandSize)
{
    if (m_commandQueueSize == 0)
    {
        pCommand->Execute();
        return;
    }

    LockQueueForNewCommand();

    void *command = FifoAllocateCommand(commandSize, false);
    Y_memcpy(command, pCommand, commandSize);

    UnlockQueueForNewCommand();
}

void CommandQueue::QueueBlockingCommand(CommandBase *pCommand, uint32 commandSize)
{
    if (m_commandQueueSize == 0 || m_workerThreads.IsEmpty())
    {
        pCommand->Execute();
        return;
    }

    // currently blocking events are only supported coming from the main thread
    DebugAssert(Thread::GetCurrentThreadId() == m_creatorThreadID);

    // lock before write
    LockQueueForNewCommand();

    // write
    void *command = FifoAllocateCommand(commandSize, true);
    Y_memcpy(command, pCommand, commandSize);

    // unlock
    UnlockQueueForNewCommand();

    // block
    m_barrier.Wait();
}

bool CommandQueue::ExecuteQueuedCommands()
{
    bool result = false;
    m_queueLock.Lock();

    // loop
    for (;;)
    {
        FifoQueueEntryHeader *commandHdr = FifoGetNextCommand();
        if (commandHdr == nullptr)
        {
            // if there's still outstanding commands, yield and search again
            if (m_activeWorkerThreads > 0 || m_activeThreadPoolTasks > 0)
            {
                m_queueLock.Unlock();
                Thread::Yield();
                m_queueLock.Lock();
                continue;
            }

            // all commands done
            break;
        }

        // unlock queue while the command runs
        m_queueLock.Unlock();

        // run command
        commandHdr->pCommand->Execute();

        // re-lock queue
        m_queueLock.Lock();

        // pop the command off the queue
        FifoReleaseCommand(commandHdr);
        result = true;
    }

    m_queueLock.Unlock();
    return result;
}

CommandQueue::WorkerThread::WorkerThread(CommandQueue *pParent)
    : m_this(pParent)
{

}

int CommandQueue::WorkerThread::ThreadEntryPoint()
{
    // initialize thread name
    Thread::SetDebugName(String::FromFormat("Command Queue %p Worker", m_this));

    // start with it locked
    m_this->m_queueLock.Lock();
    m_this->m_activeWorkerThreads++;
    MemoryBarrier();

    // loop
    for (;;)
    {
        // get the next command
        FifoQueueEntryHeader *commandHdr = m_this->FifoGetNextCommand();
        if (commandHdr == nullptr)
        {
            // no next command, are we exiting?
            if (m_this->m_workerThreadExitFlag)
                break;

            // this thread is now inactive
            m_this->m_activeWorkerThreads--;
            MemoryBarrier();

            // wait until there is a new event
            m_this->m_conditionVariable.SleepAndRelease(&m_this->m_queueLock);

            // this thread is now active
            m_this->m_activeWorkerThreads++;
            MemoryBarrier();
            continue;
        }

        // release queue lock
        m_this->m_queueLock.Unlock();

        // run the command
        commandHdr->pCommand->Execute();
        if (commandHdr->BlockingEvent)
            m_this->m_barrier.Wait();

        // hold lock again
        m_this->m_queueLock.Lock();

        // destruct the command
        m_this->FifoReleaseCommand(commandHdr);
    }

    m_this->m_activeWorkerThreads--;
    MemoryBarrier();

    m_this->m_queueLock.Unlock();
    return 0;
}

CommandQueue::ThreadPoolTask::ThreadPoolTask(CommandQueue *pParent)
    : ThreadPoolWorkItem(),
      m_this(pParent),
      m_active(false)
{

}

int32 CommandQueue::ThreadPoolTask::ProcessWork()
{
    m_this->m_queueLock.Lock();

    // loop until there are no tasks left
    for (;;)
    {
        // get the next command
        FifoQueueEntryHeader *commandHdr = m_this->FifoGetNextCommand();
        if (commandHdr == nullptr)
            break;

        // release queue lock
        m_this->m_queueLock.Unlock();

        // run the command
        commandHdr->pCommand->Execute();
        if (commandHdr->BlockingEvent)
            m_this->m_barrier.Wait();

        // hold lock again
        m_this->m_queueLock.Lock();

        // destruct the command
        m_this->FifoReleaseCommand(commandHdr);

        // check if we should yield
        if (m_this->m_threadPoolYieldToOtherJobs && m_this->m_pThreadPool->ShouldYieldToOtherTask())
            break;
    }

    // this task is no longer active
    DebugAssert(m_this->m_activeThreadPoolTasks > 0);
    m_this->m_activeThreadPoolTasks--;
    m_active = false;

    // end of this task's work
    m_this->m_queueLock.Unlock();
    return 0;
}


void CommandQueue::LockQueueForNewCommand()
{
    m_queueLock.Lock();
}

void CommandQueue::UnlockQueueForNewCommand()
{
    DebugAssert(m_commandQueue.GetSize() > 0);

    if (m_pThreadPool != nullptr)
    {
        bool queueTask = (m_activeThreadPoolTasks < m_threadPoolTasks.GetSize());
        if (queueTask)
        {
            // find a free task
            for (uint32 i = 0; i < m_threadPoolTasks.GetSize(); i++)
            {
                ThreadPoolTask *pTask = m_threadPoolTasks[i];
                if (!pTask->IsActive())
                {
                    // enqueue it
                    pTask->SetActive();
                    m_activeThreadPoolTasks++;
                    m_pThreadPool->EnqueueWorkItem(pTask);
                    break;
                }
            }
        }
        
        // release lock
        m_queueLock.Unlock();
    }
    else
    {
        if (m_activeWorkerThreads < m_workerThreads.GetSize())
            m_conditionVariable.Wake();

        m_queueLock.Unlock();
    }
}

void *CommandQueue::FifoAllocateCommand(uint32 size, bool blockingEvent)
{
    // allocate entry
    FifoQueueEntryHeader *hdr = (FifoQueueEntryHeader *)Y_malloc(sizeof(FifoQueueEntryHeader) + size);
    hdr->pCommand = reinterpret_cast<CommandBase *>(hdr + 1);
    hdr->Size = size;
    hdr->BlockingEvent = blockingEvent;
    m_commandQueue.Add(hdr);
    return hdr->pCommand;
}

bool CommandQueue::FifoIsEmpty() const
{
    return m_commandQueue.IsEmpty();
}

CommandQueue::FifoQueueEntryHeader *CommandQueue::FifoGetNextCommand()
{
    if (m_commandQueue.GetSize() == 0)
        return nullptr;

    return m_commandQueue.PopFront();
}

void CommandQueue::FifoReleaseCommand(FifoQueueEntryHeader *commandHdr)
{
    // destruct the command
    commandHdr->pCommand->~CommandBase();
    Y_free(commandHdr);
}

