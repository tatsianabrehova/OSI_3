#include <iostream>
#include <vector>
#include <windows.h>

using namespace std;

CRITICAL_SECTION cs;
HANDLE* markedEvents;
HANDLE* closeThreadEvents;
HANDLE continueEvent;
static vector<int> mas;

DWORD WINAPI marker(LPVOID params_) {
    int number = *static_cast<int*>(params_);
    int marked = 0;
    srand(number);

    EnterCriticalSection(&cs);
    while (true) {
        int random = rand();
        random %= mas.size();
        if (mas[random] == 0) {
            Sleep(5);
            mas[random] = number;
            Sleep(5);
            marked++;
        }
        else {
            cout << "Poryadkoviy nomer: " << number << std::endl;
            cout << "Kolichestvo pomechennih elementov: " << marked << std::endl;
            cout << "Index elementa kotoriy nevozmojno pometit: " << random + 1 << std::endl;
            LeaveCriticalSection(&cs);

            SetEvent(markedEvents[number - 1]);

            HANDLE* possibleOptions = new HANDLE[2];
            possibleOptions[0] = continueEvent;
            possibleOptions[1] = closeThreadEvents[number - 1];
            DWORD option = WaitForMultipleObjects(2, possibleOptions, FALSE, INFINITE);
            if (option == WAIT_OBJECT_0 + 1) {
                break;
            }
        }
    }
    for (int& i : mas) {
        if (i == number)
            i = 0;
    }

    return 0;
}

vector<HANDLE> start_threads(int count) {
    vector<HANDLE> threads_handles(count);
    for (int i = 0; i < count; i++) {
        HANDLE hThread;
        DWORD IDThread;
        int* number = new int(i + 1);
        hThread = CreateThread(
            NULL,
            0,
            marker,
            number,
            0,
            &IDThread);
        if (hThread != NULL) {
            cout << "Thread " << i + 1 << " created successfully" << endl;
            threads_handles[i] = hThread;
        }
        else {
            cout << "Something went wrong. Error code: " << GetLastError();
        }
    }
    return threads_handles;
}

HANDLE* CreateEvents(int count, bool manualReset, bool initialState) {
    HANDLE* events = new HANDLE[count];
    for (int i = 0; i < count; i++) {
        events[i] = CreateEventA(NULL, manualReset, initialState, NULL);
    }
    return events;
}

void showArray(vector<int>& v) {
    for (int i : v)
        cout << i << " ";
    cout << endl;
}

void SetRemovedEvents(vector<HANDLE>& removedEvents) {
    for (auto& removedEvent : removedEvents) {
        SetEvent(removedEvent);
    }
}

int main() {

    InitializeCriticalSection(&cs);

    cout << "Enter array size: ";
    int arr_size; cin >> arr_size;
    mas = vector<int>(arr_size, 0);

    cout << "Enter markers count: ";
    int marker_count; cin >> marker_count;

    markedEvents = CreateEvents(marker_count, FALSE, FALSE);
    continueEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    closeThreadEvents = CreateEvents(marker_count, TRUE, FALSE);
    vector<HANDLE> threads_handles = start_threads(marker_count);

    vector<HANDLE> removedMarkedEvents;

    int active_markers = marker_count;
    while (active_markers != 0) {
        SetRemovedEvents(removedMarkedEvents);
        WaitForMultipleObjects(marker_count, markedEvents, TRUE, INFINITE);
        showArray(mas);

        cout << "Enter № of thread to be closed: " << endl;
        int num; cin >> num;
        SetEvent(closeThreadEvents[num - 1]);
        WaitForSingleObject(threads_handles[num - 1], INFINITE);
        removedMarkedEvents.push_back(markedEvents[num - 1]);
        active_markers--;
        PulseEvent(continueEvent);
    }

    cout << "RESULT ARRAY" << endl;
    showArray(mas);
}
