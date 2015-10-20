/**
 * @file    main.c
 * @brief   
 */
#include <pthread.h>    // 스레드 관련 헤더
#include <stdio.h>      // 기본적은 표준 입출력 헤더
#include <stdlib.h>     // malloc, free

// 디버그 출력을 위한 설정
#if 1
#define DEBUG(msg)      printf msg
#else
#define DEBUG(msg)
#endif

#define DISPATCH_TIME       2  // 문맥전환 시간을 2 ns로 시뮬레이션한다.

/**
 * @brief   프로세스 구조체
 * @date    2015.10.19
 * @author  황규민
 */
typedef struct _process {
    int     nProcessId;         ///< 프로세스 ID
    int     nBurstTime;         ///< 버스트 타임
} ProcessInfo;

/**
 * @brief   준비큐
 * @date    2015.10.19
 * @author  황규민
 */
typedef struct _ready_queue {
    ProcessInfo             processInfo;
    struct _ready_queue     *pPrevious;
    struct _ready_queue     *pNext;         ///< 링크드리스트로 다음 프로세스 정보가 연결되어 있음
} Ready_Queue, *PReady_Queue;

PReady_Queue        g_pRootReadyQueue = NULL;          ///< 준비 큐의 Root
char                g_szResult[4096] = {'\0',}; ///< RR 스케쥴링이 저장될 문자열 변수

/**
 * @brief   프로세스의 개수를 입력받는 루틴
 * @date    2015.10.20
 * @author  황규민
 */
int
GetNumberOfProcess()
{
    char    sInputBuffer[5];                // 입력받는 문자열(프로세스의 개수)
    int     nNumberOfProcess = 0;           // 문자열을 숫자로 변경한 후 저장할 int형 변수
    
    DEBUG(("Entering %s\n", __FUNCTION__));
    
    // 어떤 정보를 입력할 것인지를 사용자에게 알림
    printf(">> 몇개의 프로세스를 스케쥴링 하겠습니까(1~999)? ");
    gets(sInputBuffer);                     // 데이터 입력
    
    // 문자열로 된 숫자를 int형 숫자로 변경
    nNumberOfProcess = atoi(sInputBuffer);
        
    // 프로세스의 기본적인 범위 설정
    if(nNumberOfProcess > 1000) {
        nNumberOfProcess = 999;
    } else if(nNumberOfProcess <=0) {
        nNumberOfProcess = 1;
    }
    
    DEBUG(("Leaving %s\n", __FUNCTION__));
        
    return nNumberOfProcess;
}

/**
 * @brief       시간 퀀텀을 입력받는 루틴
 * @date        2015.10.20
 * @author      황규민
 */
int
GetQuanterm()
{
    char    sInputBuffer[5];            // 입력받는 문자열(시간 퀀텀)
    int     nQuantomn = 0;              // 시간 퀀텀
    
    DEBUG(("Entering %s\n", __FUNCTION__));
    
    // 어떤 정보를 입력할 것인지를 사용자에게 알림
    printf(">> 작동할 시간 퀀텀을 입력하세요.(1 ns ~ 100 ns)?");
    gets(sInputBuffer);                     // 데이터 입력
    
    // 문자열로 된 숫자를 int형 숫자로 변경
    nQuantomn = atoi(sInputBuffer);
        
    // 시간 퀀텀의 기본적인 범위 설정
    if(nQuantomn > 100) {
        nQuantomn = 100;
    } else if(nQuantomn < 1) {
        nQuantomn = 1;
    }
    
    DEBUG(("Leaving %s\n", __FUNCTION__));
        
    return nQuantomn;
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // 스레드 초기화

/**
 * @brief       Round Robin Scheduler
 * @date        2015.10.20
 * @author      황규민
 */
void
*RoundRobinScheduler(void *param)
{
    PReady_Queue pCurrentProcess = g_pRootReadyQueue;   // 프로세스 처리를 위한 현재 큐
    PReady_Queue pFreeReadyQueue = NULL;               // 프로세스 처리를 위한 이전 큐
    int nQuantomn = *(int*)param;               // 시간 퀀텀을 받아온다.
    char szDoing[5];                            // 작업 프로세스
    int i;                                      // loop index
    
    DEBUG(("Entering %s(%d)\n", __FUNCTION__, nQuantomn));
    
    while(pCurrentProcess) {
        memset(szDoing, 0x00, 5);
        
        
        // 프로세스의 처리를 시각화하기 위한 작업
        sprintf(szDoing, "[%d", pCurrentProcess->processInfo.nProcessId);
        strcat(g_szResult, szDoing);
        DEBUG((szDoing));
        for(i=1;i<=nQuantomn;i++) {
            strcat(g_szResult, "-");
            DEBUG(("-"));
            // 버스트 시간을 줄이면서 프로세스의 작동을 표시한다.
            pthread_mutex_lock(&mutex);
            if(--pCurrentProcess->processInfo.nBurstTime == 0) {
                // 버스트 시간이 0이면, 처리하지 않는다.
                pthread_mutex_unlock(&mutex);
                break;
            }
            pthread_mutex_unlock(&mutex);
        }
        strcat(g_szResult, "]");
        DEBUG(("]"));
        
        // 프로세스를 이동한다.
        pthread_mutex_lock(&mutex);
        // 만약, 모든 작업이 완료되었다면, 현재의 프로세스를 준비큐에서 제거하고, 삭제한다.
        if(pCurrentProcess->processInfo.nBurstTime == 0) {
            // 모든 작업을 완료했기 때문에 레디큐에서 프로세스 정보를 제거한다.
            pFreeReadyQueue = pCurrentProcess;
            if(pCurrentProcess->pPrevious) {        // 이전 큐가 있을 경우
                pCurrentProcess->pPrevious->pNext = pCurrentProcess->pNext;
            }
            if(pCurrentProcess->pNext) {
                pCurrentProcess->pNext->pPrevious = pCurrentProcess->pPrevious;
            }
            pCurrentProcess = pCurrentProcess->pNext;
            if(pCurrentProcess == NULL) {
                if(pFreeReadyQueue == g_pRootReadyQueue) {
                    g_pRootReadyQueue = NULL;
                } else {
                    if(g_pRootReadyQueue == pFreeReadyQueue) {
                        g_pRootReadyQueue = pFreeReadyQueue->pNext;
                    }
                    pCurrentProcess = g_pRootReadyQueue;
                }
            }
            free(pFreeReadyQueue);
        } else {
            pCurrentProcess = pCurrentProcess->pNext;
            if(pCurrentProcess == NULL) {
                pCurrentProcess = g_pRootReadyQueue;
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    
    DEBUG(("Leaving %s\n", __FUNCTION__));
}

/**
 * @breif       레디큐에 하나의 프로세스 정보를 추가한다.
 * @param[in]   pReadyQueue     추가할 프로세스 정보를 가지고 있는 큐객체
 * @return      void*           레디큐에 연결 후의 해당 엔트리 포인터
 * @date        2015.10.20
 * @author      황규민
 */
void*
AddtoReadyQueue(PReady_Queue pReadyQueue)
{
    PReady_Queue    pTempReadyQueue = NULL; // 임시 레디큐 객체
    
    // 임계영역 처리를 위해 mutex로 lock을 건다.
    pthread_mutex_lock(&mutex);
    if(g_pRootReadyQueue) {
        // 레디큐의 맨 마지막으로 간다.
        pTempReadyQueue = g_pRootReadyQueue;
        while(pTempReadyQueue->pNext) {
            pTempReadyQueue = pTempReadyQueue->pNext;
        }
        
        // 레디큐의 맨 마지막에서 데이터를 추가한다.
        pTempReadyQueue->pNext = pReadyQueue;
        pReadyQueue->pPrevious = pTempReadyQueue;
    } else {
        // 레디큐의 루트가 비어있을 경우 대체한다.
        g_pRootReadyQueue = pReadyQueue;
    }
    pthread_mutex_unlock(&mutex);   // unlock한다.
    
    return pReadyQueue;
}

/**
 * @brief       메인 루틴
 * @date        2015.10.20
 * @author      황규민
 */
void
main(int argc, char **argv) {
    
    int             nNumberOfProcess;       // 프로세스의 개수
    int             nQuantomn;              // 문맥전환이 일어날 시간 퀀텀
    PReady_Queue    pReadyQueue;            // ready queue 를 할당할 포인터
    char            sInputBuffer[10];       // 데이터 입력을 위한 입력 버퍼
    pthread_t       tid;                    // 스레드 ID
    pthread_attr_t  attr;                   // 스레드 속성
    int             i;                      // loop index
    PReady_Queue    pTempReady_Queue = NULL;
        
    DEBUG(("Entering %s\n", __FUNCTION__));
    
    if(argc >= 2) {
        // 파라메터에서 프로세스 개수를 가져오고, 그 값이 유효한지 검증한다.
        nNumberOfProcess = atoi(argv[1]);
        if(nNumberOfProcess > 1000) {
            nNumberOfProcess = 999;
        } else if(nNumberOfProcess <=0) {
            nNumberOfProcess = 1;
        }
    } else {
        nNumberOfProcess = GetNumberOfProcess();
    }
    printf(">> %d 개의 프로세스를 스케쥴링 합니다.\n", nNumberOfProcess);
    
    for(i=1;i<=nNumberOfProcess;i++) {
        printf(">> %d 번째 프로세스의 버스트 시간을 입력하세요? ", i);
        gets(sInputBuffer);
        pReadyQueue = malloc(sizeof(Ready_Queue));                       // 프로세스를 할당한다.
        if(pReadyQueue) {
            pReadyQueue->processInfo.nBurstTime = atoi(sInputBuffer);    // 버스트 시간 
            pReadyQueue->processInfo.nProcessId = i;
            pReadyQueue->pNext = NULL;
            pReadyQueue->pPrevious = NULL;
            
            AddtoReadyQueue(pReadyQueue);
        }
    }
    
    if(argc >= 3) {
        // 파라메터에서 시간 퀀텀을 가져오고, 그 값이 유효한지 검증한다.
        nQuantomn = atoi(argv[2]);
        if(nQuantomn > 100) {
            nQuantomn = 100;
        } else if(nQuantomn < 1) {
            nQuantomn = 1;
        }
    } else {
        nQuantomn = GetQuanterm();
    }
    
    printf(">> 시간 퀀텀은 %d ns 입니다.\n", nQuantomn);
    
    // Round Robin 스케쥴링 스레드 생성
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, RoundRobinScheduler, &nQuantomn);
    pthread_join(tid, NULL);        // Round Robin 스케쥴러가 끝날때까지 대기
    
    printf("Round Robin의 스케쥴 결과는 아래와 같습니다.\n");
    printf("%s\n", g_szResult);
    
    DEBUG(("Leaving %s\n", __FUNCTION__));
}
