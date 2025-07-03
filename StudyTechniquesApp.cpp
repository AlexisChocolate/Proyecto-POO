#define _WIN32_IE 0x0501
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <random>
#include <commctrl.h> // Para ListView y GroupBox
#include <cstdio> // For sprintf_s

#pragma comment(lib, "comctl32.lib")

using namespace std;


#define IDC_SPACED_PRACTICE_BUTTON 101
#define IDC_ACTIVE_RECALL_BUTTON 102
#define IDC_INTERLEAVING_BUTTON 103
#define IDC_RESULTS_BUTTON 104
#define IDC_EXIT_BUTTON 105


#define IDC_TOPIC_EDIT 201
#define IDC_SCHEDULE_BUTTON 202
#define IDC_REPASOS_LISTVIEW 203
#define IDC_DELETE_REPASO_BUTTON 204


#define IDC_QUESTION_EDIT 301
#define IDC_ANSWER_EDIT 302
#define IDC_SAVE_FLASHCARD_BUTTON 303
#define IDC_FLASHCARDS_LIST 304
#define IDC_PRACTICE_BUTTON 305


#define IDC_PRACTICE_QUESTION_LABEL 401
#define IDC_PRACTICE_ANSWER_EDIT 402
#define IDC_VERIFY_ANSWER_BUTTON 403
#define IDC_CORRECT_ANSWER_LABEL 404
#define IDC_NEXT_FLASHCARD_BUTTON 405


#define IDC_SUBJECT_LIST_INTERLEAVING 501
#define IDC_ADD_SUBJECT_EDIT 502
#define IDC_ADD_SUBJECT_BUTTON 503
#define IDC_DELETE_SUBJECT_BUTTON 504
#define IDC_STUDY_TIME_EDIT 505
#define IDC_BREAK_TIME_EDIT 506
#define IDC_TIMER_LABEL 507
#define IDC_STATUS_LABEL 508
#define IDC_START_PAUSE_BUTTON 509
#define IDC_STOP_BUTTON 510


#define IDC_RESULTS_LABEL 601


#define IDC_BACK_BUTTON 999




LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SpacedPracticeWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ActiveRecallWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PracticeWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InterleavingWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ResultsWndProc(HWND, UINT, WPARAM, LPARAM);


void RegisterSpacedPracticeWindow(HINSTANCE, HBRUSH);
void DisplaySpacedPracticeWindow(HWND, HINSTANCE);
void RegisterActiveRecallWindow(HINSTANCE, HBRUSH);
void DisplayActiveRecallWindow(HWND, HINSTANCE);
void RegisterPracticeWindow(HINSTANCE, HBRUSH);
void DisplayPracticeWindow(HWND, HINSTANCE);
void RegisterInterleavingWindow(HINSTANCE, HBRUSH);
void DisplayInterleavingWindow(HWND, HINSTANCE);
void RegisterResultsWindow(HINSTANCE, HBRUSH);
void DisplayResultsWindow(HWND, HINSTANCE);


void LoadFlashcards();
void SaveSubjects();
void LoadSubjects();
void LoadSubjectsToListbox(HWND);
void UpdateTimerLabel(HWND);
void UpdateStatusLabel(HWND);
void ShowNextFlashcard(HWND);
void LoadFlashcardsToListbox(HWND);
void LoadRepasosToListView(HWND);
void UpdateRepasoStatusInFile(int, bool);
void DeleteRepaso(HWND, int);
int CountLines(const string&);


void CenterWindow(HWND, HWND);
bool ValidateInput(const char*, const char*, HWND);
void CreateBackup();
string GetDetailedStats();
void ExportToCSV();
void LogError(const string&);

struct AppConfig {
    int pomodoro_work_time = 25;
    int pomodoro_break_time = 5;
    bool auto_backup = true;
    int review_intervals[5] = {1, 3, 7, 15, 30};
    string app_theme = "default";
};
extern AppConfig g_config;
void LoadConfig();
void SaveConfig();

struct StudyProgress {
    int total_flashcards_reviewed = 0;
    int total_pomodoros_completed = 0;
    int total_topics_reviewed = 0;
    time_t last_study_session = 0;
};
extern StudyProgress g_progress;
void UpdateProgress(const string&);
void LoadProgress();
void SaveProgress();




struct Flashcard {
    string question;
    string answer;
};

vector<Flashcard> g_flashcards;
int g_currentFlashcardIndex = -1;

void LoadFlashcards() {
    g_flashcards.clear();
    ifstream infile("flashcards.txt");
    string line;
    while (getline(infile, line)) {
        size_t separator_pos = line.find('|');
        if (separator_pos != string::npos) {
            Flashcard fc;
            fc.question = line.substr(0, separator_pos);
            fc.answer = line.substr(separator_pos + 1);
            g_flashcards.push_back(fc);
        }
    }
    infile.close();
}


AppConfig g_config;
StudyProgress g_progress;




void CenterWindow(HWND hwnd, HWND parent) {
    RECT rectParent, rectChild;
    GetWindowRect(parent, &rectParent);
    GetWindowRect(hwnd, &rectChild);
    
    int width = rectChild.right - rectChild.left;
    int height = rectChild.bottom - rectChild.top;
    
    int x = rectParent.left + (rectParent.right - rectParent.left - width) / 2;
    int y = rectParent.top + (rectParent.bottom - rectParent.top - height) / 2;
    
    SetWindowPos(hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
}


bool ValidateInput(const char* input, const char* fieldName, HWND parent) {
    if (strlen(input) == 0) {
        string message = "El campo '" + string(fieldName) + "' no puede estar vacio.";
        MessageBox(parent, message.c_str(), "Error de Validacion", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (strlen(input) > 500) {
        string message = "El campo '" + string(fieldName) + "' es demasiado largo (maximo 500 caracteres).";
        MessageBox(parent, message.c_str(), "Error de Validacion", MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}


void CreateBackup() {
    time_t now = time(0);
    char backup_suffix[20];
    strftime(backup_suffix, sizeof(backup_suffix), "%Y%m%d_%H%M%S", localtime(&now));
    
    string files[] = {"repasos.txt", "flashcards.txt", "materias.txt"};
    for (const string& file : files) {
        string backup_name = file.substr(0, file.find('.')) + "_backup_" + backup_suffix + ".txt";
        CopyFile(file.c_str(), backup_name.c_str(), FALSE);
    }
}

string GetCurrentDateString() {
    time_t now = time(0);
    char date_str[100];
    strftime(date_str, sizeof(date_str), "%d/%m/%Y", localtime(&now));
    return string(date_str);
}


string GetDetailedStats() {
    string stats = "=== ESTADISTICAS DETALLADAS ===\n\n";
    
    
    int repasos_count = 0;
    ifstream repasos_file("repasos.txt");
    string line;
    while (getline(repasos_file, line)) {
        if (line.find("Tema: ") == 0) {
            repasos_count++;
        }
    }
    repasos_file.close();
    
    
    int flashcards_count = CountLines("flashcards.txt");
    
    
    int materias_count = CountLines("materias.txt");
    
    stats += " PRACTICA ESPACIADA:\n";
    stats += "  • Temas programados: " + to_string(repasos_count) + "\n";
    stats += "  • Promedio de repasos por tema: 5\n\n";
    
    stats += " RECUPERACION ACTIVA:\n";
    stats += "  • Flashcards creadas: " + to_string(flashcards_count) + "\n";
    stats += "  • Promedio de palabras por flashcard: ";
    
    
    if (flashcards_count > 0) {
        int total_words = 0;
        ifstream fc_file("flashcards.txt");
        while (getline(fc_file, line)) {
            int words = 1;
            for (char c : line) {
                if (c == ' ') words++;
            }
            total_words += words;
        }
        fc_file.close();
        stats += to_string(total_words / flashcards_count);
    } else {
        stats += "0";
    }
    stats += "\n\n";
    
    stats += " INTERLEAVING:\n";
    stats += "  • Materias registradas: " + to_string(materias_count) + "\n";
    stats += "  • Tiempo recomendado por sesion: 25 min\n\n";
    
    stats += " RESUMEN GENERAL:\n";
    stats += "  • Total de elementos: " + to_string(repasos_count + flashcards_count + materias_count) + "\n";
    stats += "  • Tecnicas en uso: 3/3\n";
    stats += "  • Estado del sistema: Activo \xFB\n";
    
    return stats;
}


void ExportToCSV() {
    ofstream export_file("study_data_export.csv");
    export_file << "Tipo,Contenido,Fecha_Creacion\n";
    
    
    ifstream repasos("repasos.txt");
    string line;
    while (getline(repasos, line)) {
        if (line.find("Tema: ") == 0) {
            string tema = line.substr(6);
                        export_file << "Repaso,\"" << tema << "\",\"" << GetCurrentDateString() << "\"\n";
        }
    }
    repasos.close();
    
    
    ifstream flashcards("flashcards.txt");
    while (getline(flashcards, line)) {
        size_t pos = line.find('|');
        if (pos != string::npos) {
            string pregunta = line.substr(0, pos);
            string respuesta = line.substr(pos + 1);
            export_file << "Flashcard,\"" << pregunta << " | " << respuesta << "\",\"" << GetCurrentDateString() << "\"\n";
        }
    }
    flashcards.close();
    
    
    ifstream materias("materias.txt");
    while (getline(materias, line)) {
        export_file << "Materia,\"" << line << ",\"" << GetCurrentDateString() << "\"\n";
    }
    materias.close();
    
    export_file.close();
        MessageBox(NULL, "datos exportados exitosamente a 'study_data_export.csv'", "exportacion completa", MB_OK | MB_ICONINFORMATION);
}


void LoadConfig() {
    ifstream config_file("config.ini");
    if (config_file.is_open()) {
        string line;
        while (getline(config_file, line)) {
            if (line.find("work_time=") == 0) {
                g_config.pomodoro_work_time = stoi(line.substr(10));
            }
            else if (line.find("break_time=") == 0) {
                g_config.pomodoro_break_time = stoi(line.substr(11));
            }
            else if (line.find("auto_backup=") == 0) {
                g_config.auto_backup = (line.substr(12) == "true");
            }
        }
        config_file.close();
    }
}

void SaveConfig() {
    ofstream config_file("config.ini");
    config_file << "work_time=" << g_config.pomodoro_work_time << "\n";
    config_file << "break_time=" << g_config.pomodoro_break_time << "\n";
    config_file << "auto_backup=" << (g_config.auto_backup ? "true" : "false") << "\n";
    config_file.close();
}


void LogError(const string& error_msg) {
    ofstream log_file("error_log.txt", ios_base::app);
    time_t now = time(0);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    
    log_file << "[" << time_str << "] ERROR: " << error_msg << "\n";
    log_file.close();
}


void LoadProgress() {
    ifstream progress_file("progress.dat", ios::binary);
    if (progress_file.is_open()) {
        progress_file.read((char*)&g_progress, sizeof(StudyProgress));
        progress_file.close();
    }
}

void SaveProgress() {
    ofstream progress_file("progress.dat", ios::binary);
    progress_file.write((char*)&g_progress, sizeof(StudyProgress));
    progress_file.close();
}

void UpdateProgress(const string& activity_type) {
    if (activity_type == "flashcard") {
        g_progress.total_flashcards_reviewed++;
    }
    else if (activity_type == "pomodoro") {
        g_progress.total_pomodoros_completed++;
    }
    else if (activity_type == "topic") {
        g_progress.total_topics_reviewed++;
    }
    
    g_progress.last_study_session = time(0);
    
    
    SaveProgress();
}




const char g_szResultsClassName[] = "ResultsWindowClass";

void RegisterResultsWindow(HINSTANCE hInstance, HBRUSH hbrBackground) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = ResultsWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szResultsClassName;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
}

void DisplayResultsWindow(HWND hWnd, HINSTANCE hInstance) {
    HWND hwnd = CreateWindowEx(
        0, g_szResultsClassName, TEXT("Resultados"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 400, // Aumentar alto para el botón
        hWnd, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hWnd, "No se pudo crear la ventana de Resultados.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);
}

int CountLines(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) return 0;
    int count = 0;
    string line;
    while (getline(file, line)) {
        if (!line.empty()) {
            count++;
        }
    }
    return count;
}

LRESULT CALLBACK ResultsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));
            HFONT hHeaderFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            string detailed_stats = GetDetailedStats();
            
            CreateWindow(TEXT("STATIC"), detailed_stats.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 20, 400, 300, hwnd, (HMENU)IDC_RESULTS_LABEL, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Exportar Datos"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 20, 340, 120, 30, hwnd, (HMENU)701, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Volver al Menu"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 150, 340, 120, 30, hwnd, (HMENU)IDC_BACK_BUTTON, NULL, NULL);
            
            SendMessage(GetDlgItem(hwnd, IDC_RESULTS_LABEL), WM_SETFONT, (WPARAM)hFont, TRUE);
            
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BACK_BUTTON) {
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 701) {
                ExportToCSV();
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}



const char g_szInterleavingClassName[] = "InterleavingWindowClass";
#define ID_TIMER 1

enum StudyState { STUDY, BREAK };

struct Pomodoro {
    int study_minutes = 25;
    int break_minutes = 5;
    int time_left_seconds = 25 * 60;
    StudyState current_state = STUDY;
    bool is_running = false;
    vector<string> subjects;
    int current_subject_index = 0;
};

Pomodoro g_pomodoro;

void LoadSubjects() {
    g_pomodoro.subjects.clear();
    ifstream infile("materias.txt");
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) {
            g_pomodoro.subjects.push_back(line);
        }
    }
    infile.close();
}

void SaveSubjects() {
    ofstream outfile("materias.txt");
    for (const auto& subject : g_pomodoro.subjects) {
        outfile << subject << endl;
    }
    outfile.close();
}

void LoadSubjectsToListbox(HWND hList) {
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    for (const auto& subject : g_pomodoro.subjects) {
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)subject.c_str());
    }
}


void RegisterInterleavingWindow(HINSTANCE hInstance, HBRUSH hbrBackground) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = InterleavingWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szInterleavingClassName;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
}

void DisplayInterleavingWindow(HWND hWnd, HINSTANCE hInstance) {
    LoadSubjects();
    HWND hwnd = CreateWindowEx(
        0, g_szInterleavingClassName, TEXT("Interleaving Pomodoro"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 550, 650, // Aumentar alto
        hWnd, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hWnd, "No se pudo crear la ventana de Interleaving.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);
    CenterWindow(hwnd, hWnd);
}

void UpdateTimerLabel(HWND hwnd) {
    int minutes = g_pomodoro.time_left_seconds / 60;
    int seconds = g_pomodoro.time_left_seconds % 60;
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", minutes, seconds);
    SetDlgItemText(hwnd, IDC_TIMER_LABEL, time_str);
}

void UpdateStatusLabel(HWND hwnd) {
    string status_text;
    if (g_pomodoro.current_state == STUDY) {
        status_text = "Estudiando: ";
        if (!g_pomodoro.subjects.empty()) {
            if (g_pomodoro.current_subject_index >= g_pomodoro.subjects.size()) {
                g_pomodoro.current_subject_index = 0;
            }
            status_text += g_pomodoro.subjects[g_pomodoro.current_subject_index];
        } else {
            status_text += "(Anade una materia)";
        }
    } else {
        status_text = "Descanso";
    }
    SetDlgItemText(hwnd, IDC_STATUS_LABEL, status_text.c_str());
}

LRESULT CALLBACK InterleavingWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hStudyTimeEdit, hBreakTimeEdit, hSubjectList, hAddSubjectEdit;
    static DWORD s_lastTickCount = 0;
    static DWORD s_accumulated_elapsed_ms = 0;

    switch (msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));
            HFONT hTimerFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            CreateWindow(TEXT("BUTTON"), TEXT("Gestion de Materias"), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, 10, 500, 220, hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Nueva Materia:"), WS_VISIBLE | WS_CHILD, 30, 40, 150, 20, hwnd, NULL, NULL, NULL);
            hAddSubjectEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 150, 40, 220, 25, hwnd, (HMENU)IDC_ADD_SUBJECT_EDIT, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Anadir"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 380, 40, 120, 25, hwnd, (HMENU)IDC_ADD_SUBJECT_BUTTON, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Materias Actuales:"), WS_VISIBLE | WS_CHILD, 30, 80, 200, 20, hwnd, NULL, NULL, NULL);
            hSubjectList = CreateWindow(TEXT("LISTBOX"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 30, 105, 340, 100, hwnd, (HMENU)IDC_SUBJECT_LIST_INTERLEAVING, NULL, NULL);
                        CreateWindow(TEXT("BUTTON"), TEXT("Eliminar"), WS_VISIBLE | WS_CHILD, 380, 105, 120, 25, hwnd, (HMENU)IDC_DELETE_SUBJECT_BUTTON, NULL, NULL);
            LoadSubjectsToListbox(hSubjectList);

            CreateWindow(TEXT("BUTTON"), TEXT("Temporizador Pomodoro"), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, 240, 500, 290, hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Tiempo de Estudio (min):"), WS_VISIBLE | WS_CHILD, 30, 270, 200, 25, hwnd, NULL, NULL, NULL);
            hStudyTimeEdit = CreateWindow(TEXT("EDIT"), TEXT("25"), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 240, 270, 50, 25, hwnd, (HMENU)IDC_STUDY_TIME_EDIT, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Tiempo de Descanso (min):"), WS_VISIBLE | WS_CHILD, 30, 305, 200, 25, hwnd, NULL, NULL, NULL);
            hBreakTimeEdit = CreateWindow(TEXT("EDIT"), TEXT("5"), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER, 240, 305, 50, 25, hwnd, (HMENU)IDC_BREAK_TIME_EDIT, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("25:00"), WS_VISIBLE | WS_CHILD | SS_CENTER, 165, 350, 200, 60, hwnd, (HMENU)IDC_TIMER_LABEL, NULL, NULL);
            SendMessage(GetDlgItem(hwnd, IDC_TIMER_LABEL), WM_SETFONT, (WPARAM)hTimerFont, TRUE);
            CreateWindow(TEXT("STATIC"), TEXT("Estado"), WS_VISIBLE | WS_CHILD | SS_CENTER, 165, 420, 200, 25, hwnd, (HMENU)IDC_STATUS_LABEL, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Iniciar/Pausar"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 125, 470, 150, 40, hwnd, (HMENU)IDC_START_PAUSE_BUTTON, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Detener"), WS_VISIBLE | WS_CHILD, 295, 470, 150, 40, hwnd, (HMENU)IDC_STOP_BUTTON, NULL, NULL);
            
            CreateWindow(TEXT("BUTTON"), TEXT("Volver al Menu"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 185, 550, 150, 40, hwnd, (HMENU)IDC_BACK_BUTTON, NULL, NULL);

            SendMessage(hAddSubjectEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hSubjectList, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hwnd, IDC_STATUS_LABEL), WM_SETFONT, (WPARAM)hFont, TRUE);
            UpdateStatusLabel(hwnd);
            break;
        }
        case WM_TIMER: {
            if (wParam == ID_TIMER && g_pomodoro.is_running) {
                DWORD currentTickCount = GetTickCount();
                DWORD elapsed = currentTickCount - s_lastTickCount;
                s_lastTickCount = currentTickCount;

                s_accumulated_elapsed_ms += elapsed;

                while (s_accumulated_elapsed_ms >= 1000) {
                    g_pomodoro.time_left_seconds--;
                    s_accumulated_elapsed_ms -= 1000;
                }

                
                    if (g_pomodoro.time_left_seconds <= 0) {
                    Beep(784, 200); // Reinsertar el Beep
                    if (g_pomodoro.current_state == STUDY) {
                        g_pomodoro.current_state = BREAK;
                        g_pomodoro.time_left_seconds = g_pomodoro.break_minutes * 60;
                    } else {
                        g_pomodoro.current_state = STUDY;
                        g_pomodoro.time_left_seconds = g_pomodoro.study_minutes * 60;
                        if (!g_pomodoro.subjects.empty()) {
                            g_pomodoro.current_subject_index = (g_pomodoro.current_subject_index + 1) % g_pomodoro.subjects.size();
                        }
                    }
                    UpdateStatusLabel(hwnd);
                }
                UpdateTimerLabel(hwnd);
            }
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_BACK_BUTTON: DestroyWindow(hwnd); break;
                case IDC_ADD_SUBJECT_BUTTON: {
                    char subject[256];
                    GetWindowText(hAddSubjectEdit, subject, 256);
                    if (strlen(subject) > 0) {
                        g_pomodoro.subjects.push_back(subject);
                        SaveSubjects();
                        LoadSubjectsToListbox(GetDlgItem(hwnd, IDC_SUBJECT_LIST_INTERLEAVING));
                        SetWindowText(hAddSubjectEdit, "");
                        UpdateStatusLabel(hwnd);
                    }
                    break;
                }
                case IDC_DELETE_SUBJECT_BUTTON: {
                    HWND hList = GetDlgItem(hwnd, IDC_SUBJECT_LIST_INTERLEAVING);
                    int sel = SendMessage(hList, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR) {
                        g_pomodoro.subjects.erase(g_pomodoro.subjects.begin() + sel);
                        SaveSubjects();
                        LoadSubjectsToListbox(hList);
                        UpdateStatusLabel(hwnd);
                    }
                    break;
                }
                case IDC_START_PAUSE_BUTTON: {
                    if (g_pomodoro.is_running) {
                        g_pomodoro.is_running = false;
                        KillTimer(hwnd, ID_TIMER);
                    } else {
                        if (g_pomodoro.subjects.empty()) {
                            MessageBox(hwnd, "Por favor, anada al menos una materia para estudiar.", "Aviso", MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                        char study_time_str[10], break_time_str[10];
                        GetWindowText(hStudyTimeEdit, study_time_str, 10);
                        GetWindowText(hBreakTimeEdit, break_time_str, 10);
                        g_pomodoro.study_minutes = atoi(study_time_str);
                        g_pomodoro.break_minutes = atoi(break_time_str);
                        if (!g_pomodoro.is_running) {
                             g_pomodoro.time_left_seconds = g_pomodoro.study_minutes * 60;
                        }
                        g_pomodoro.is_running = true;
                        SetTimer(hwnd, ID_TIMER, 100, NULL);
                        s_lastTickCount = GetTickCount();
                        s_accumulated_elapsed_ms = 0;
                        UpdateStatusLabel(hwnd);
                        UpdateTimerLabel(hwnd);
                    }
                    break;
                }
                case IDC_STOP_BUTTON: {
                    g_pomodoro.is_running = false;
                    KillTimer(hwnd, ID_TIMER);
                    char study_time_str[10];
                    GetWindowText(hStudyTimeEdit, study_time_str, 10);
                    g_pomodoro.study_minutes = atoi(study_time_str);
                    g_pomodoro.time_left_seconds = g_pomodoro.study_minutes * 60;
                    g_pomodoro.current_state = STUDY;
                    g_pomodoro.current_subject_index = 0;
                    UpdateTimerLabel(hwnd);
                    UpdateStatusLabel(hwnd);
                    break;
                }
            }
            break;
        }
        case WM_CLOSE:
            KillTimer(hwnd, ID_TIMER);
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}



const char g_szPracticeClassName[] = "PracticeWindowClass";

void RegisterPracticeWindow(HINSTANCE hInstance, HBRUSH hbrBackground) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = PracticeWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szPracticeClassName;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
}

void DisplayPracticeWindow(HWND hWnd, HINSTANCE hInstance) {
    LoadFlashcards();
    if (g_flashcards.empty()) {
        MessageBox(hWnd, "No hay flashcards para practicar. Agregue algunas primero.", "Informacion", MB_OK | MB_ICONINFORMATION);
        return;
    }

    HWND hwnd = CreateWindowEx(
        0, g_szPracticeClassName, TEXT("Practicar Recuperacion Activa"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 550,
        hWnd, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hWnd, "No se pudo crear la ventana de practica.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);
    CenterWindow(hwnd, hWnd);
}

void ShowNextFlashcard(HWND hwnd) {
    if (g_flashcards.empty()) return;
    g_currentFlashcardIndex = rand() % g_flashcards.size();
    SetDlgItemText(hwnd, IDC_PRACTICE_QUESTION_LABEL, g_flashcards[g_currentFlashcardIndex].question.c_str());
    SetDlgItemText(hwnd, IDC_PRACTICE_ANSWER_EDIT, "");
    SetDlgItemText(hwnd, IDC_CORRECT_ANSWER_LABEL, "");
}

LRESULT CALLBACK PracticeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            CreateWindow(TEXT("STATIC"), TEXT("Pregunta:"), WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("STATIC"), "", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT, 20, 45, 540, 50, hwnd, (HMENU)IDC_PRACTICE_QUESTION_LABEL, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Tu Respuesta:"), WS_VISIBLE | WS_CHILD, 20, 115, 150, 20, hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, 20, 140, 540, 100, hwnd, (HMENU)IDC_PRACTICE_ANSWER_EDIT, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Verificar Respuesta"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 200, 250, 180, 30, hwnd, (HMENU)IDC_VERIFY_ANSWER_BUTTON, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Respuesta Correcta:"), WS_VISIBLE | WS_CHILD, 20, 300, 200, 20, hwnd, NULL, NULL, NULL);
            CreateWindow(TEXT("STATIC"), "", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT, 20, 325, 540, 80, hwnd, (HMENU)IDC_CORRECT_ANSWER_LABEL, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Siguiente Flashcard"), WS_VISIBLE | WS_CHILD, 200, 415, 180, 30, hwnd, (HMENU)IDC_NEXT_FLASHCARD_BUTTON, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Volver al Menu"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 200, 465, 180, 40, hwnd, (HMENU)IDC_BACK_BUTTON, NULL, NULL);

            SendMessage(GetDlgItem(hwnd, IDC_PRACTICE_QUESTION_LABEL), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hwnd, IDC_PRACTICE_ANSWER_EDIT), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hwnd, IDC_CORRECT_ANSWER_LABEL), WM_SETFONT, (WPARAM)hFont, TRUE);

            srand(time(NULL));
            ShowNextFlashcard(hwnd);
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_BACK_BUTTON: DestroyWindow(hwnd); break;
                case IDC_VERIFY_ANSWER_BUTTON: {
                    if (g_currentFlashcardIndex != -1) {
                        SetDlgItemText(hwnd, IDC_CORRECT_ANSWER_LABEL, g_flashcards[g_currentFlashcardIndex].answer.c_str());
                    }
                    break;
                }
                case IDC_NEXT_FLASHCARD_BUTTON: {
                    ShowNextFlashcard(hwnd);
                    break;
                }
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


// --- Ventana de Recuperación Activa ---

const char g_szActiveRecallClassName[] = "ActiveRecallWindowClass";

void RegisterActiveRecallWindow(HINSTANCE hInstance, HBRUSH hbrBackground) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = ActiveRecallWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szActiveRecallClassName;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
}

void DisplayActiveRecallWindow(HWND hWnd, HINSTANCE hInstance) {
    HWND hwnd = CreateWindowEx(
        0, g_szActiveRecallClassName, TEXT("Recuperacion Activa - Gestion de Flashcards"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 550,
        hWnd, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hWnd, "No se pudo crear la ventana de Recuperacion Activa.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);
    CenterWindow(hwnd, hWnd);
}

void LoadFlashcardsToListbox(HWND hList) {
    SendMessage(hList, LB_RESETCONTENT, 0, 0);
    ifstream infile("flashcards.txt");
    string line;
    while (getline(infile, line)) {
        size_t separator_pos = line.find('|');
        if (separator_pos != string::npos) {
            string question = line.substr(0, separator_pos);
            SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)question.c_str());
        }
    }
    infile.close();
}

LRESULT CALLBACK ActiveRecallWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hQuestionEdit, hAnswerEdit, hFlashcardsList;

    switch (msg) {
        case WM_CREATE: {
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            CreateWindow(TEXT("STATIC"), TEXT("Pregunta:"), WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
            hQuestionEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, 20, 45, 440, 60, hwnd, (HMENU)IDC_QUESTION_EDIT, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Respuesta:"), WS_VISIBLE | WS_CHILD, 20, 115, 100, 20, hwnd, NULL, NULL, NULL);
            hAnswerEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, 20, 140, 440, 60, hwnd, (HMENU)IDC_ANSWER_EDIT, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Guardar Flashcard"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 165, 210, 150, 30, hwnd, (HMENU)IDC_SAVE_FLASHCARD_BUTTON, NULL, NULL);
            CreateWindow(TEXT("STATIC"), TEXT("Flashcards Existentes:"), WS_VISIBLE | WS_CHILD, 20, 260, 200, 20, hwnd, NULL, NULL, NULL);
            hFlashcardsList = CreateWindow(TEXT("LISTBOX"), NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 20, 285, 440, 100, hwnd, (HMENU)IDC_FLASHCARDS_LIST, NULL, NULL);
            LoadFlashcardsToListbox(hFlashcardsList);
            CreateWindow(TEXT("BUTTON"), TEXT("Practicar"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 165, 405, 150, 40, hwnd, (HMENU)IDC_PRACTICE_BUTTON, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Eliminar Flashcard"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 325, 405, 150, 40, hwnd, (HMENU)306, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Volver al Menu"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 165, 465, 150, 40, hwnd, (HMENU)IDC_BACK_BUTTON, NULL, NULL);

            SendMessage(hQuestionEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hAnswerEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hFlashcardsList, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_BACK_BUTTON: DestroyWindow(hwnd); break;
                case IDC_SAVE_FLASHCARD_BUTTON: {
                    char question[1024], answer[1024];
                    GetWindowText(hQuestionEdit, question, 1024);
                    GetWindowText(hAnswerEdit, answer, 1024);
                    if (!ValidateInput(question, "Pregunta", hwnd) || !ValidateInput(answer, "Respuesta", hwnd)) {
                        return 0;
                    }
                    ofstream outfile("flashcards.txt", ios_base::app);
                    outfile << question << "|" << answer << endl;
                    outfile.close();
                    SendMessage(hQuestionEdit, WM_SETTEXT, 0, (LPARAM)"");
                    SendMessage(hAnswerEdit, WM_SETTEXT, 0, (LPARAM)"");
                    LoadFlashcardsToListbox(hFlashcardsList);
                    MessageBox(hwnd, "Flashcard guardada.", "Exito", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                case IDC_PRACTICE_BUTTON: {
                     DisplayPracticeWindow(hwnd, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
                     break;
                }
                case 306: {
                    int sel = SendMessage(hFlashcardsList, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR) {
                        vector<string> lines;
                        ifstream infile("flashcards.txt");
                        string line;
                        while(getline(infile, line)) {
                            lines.push_back(line);
                        }
                        infile.close();

                        if (sel >= 0 && sel < lines.size()) {
                            lines.erase(lines.begin() + sel);
                        }

                        ofstream outfile("flashcards.txt");
                        for(const auto& l : lines) {
                            outfile << l << endl;
                        }
                        outfile.close();
                        LoadFlashcardsToListbox(hFlashcardsList);
                        MessageBox(hwnd, "Flashcard eliminada.", "Exito", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBox(hwnd, "Por favor, seleccione una flashcard para eliminar.", "Advertencia", MB_OK | MB_ICONWARNING);
                    }
                    break;
                }
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}




const char g_szSpacedPracticeClassName[] = "SpacedPracticeWindowClass";

void RegisterSpacedPracticeWindow(HINSTANCE hInstance, HBRUSH hbrBackground) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = SpacedPracticeWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = g_szSpacedPracticeClassName;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
}

void DisplaySpacedPracticeWindow(HWND hWnd, HINSTANCE hInstance) {
    HWND hwnd = CreateWindowEx(
        0, g_szSpacedPracticeClassName, TEXT("Practica Espaciada"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 550,
        hWnd, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(hWnd, "No se pudo crear la ventana de Practica Espaciada.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    ShowWindow(hwnd, SW_SHOW);
    CenterWindow(hwnd, hWnd);
}

void LoadRepasosToListView(HWND hListView) {
    ListView_DeleteAllItems(hListView);
    ifstream infile("repasos.txt");
    string line;
    int itemIndex = 0;
    while (getline(infile, line)) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_STATE;
        lvi.iItem = itemIndex;
        lvi.iSubItem = 0;
        
        bool isChecked = false;
        if (line.rfind("[x] ", 0) == 0) {
            isChecked = true;
            line = line.substr(4); // Remove "[x] "
        }

        lvi.pszText = (LPSTR)line.c_str();
        ListView_InsertItem(hListView, &lvi);
        ListView_SetCheckState(hListView, itemIndex, isChecked);
        itemIndex++;
    }
    infile.close();
}

void UpdateRepasoStatusInFile(int itemIndex, bool isChecked) {
    vector<string> lines;
    ifstream infile("repasos.txt");
    string line;
    while(getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    if (itemIndex >= 0 && itemIndex < lines.size()) {
        string& targetLine = lines[itemIndex];
        if (isChecked) {
            if (targetLine.rfind("[x] ", 0) != 0) {
                targetLine.insert(0, "[x] ");
            }
        } else {
            if (targetLine.rfind("[x] ", 0) == 0) {
                targetLine.erase(0, 4);
            }
        }
    }

    ofstream outfile("repasos.txt");
    for(const auto& l : lines) {
        outfile << l << endl;
    }
    outfile.close();
}

void DeleteRepaso(HWND hwndListView, int itemIndex) {
    vector<string> lines;
    ifstream infile("repasos.txt");
    string line;
    while(getline(infile, line)) {
        lines.push_back(line);
    }
    infile.close();

    if (itemIndex >= 0 && itemIndex < lines.size()) {
        // A Repaso consists of 6 lines (Tema: + 5 Repaso: lines)
        // Find the start of the current repaso block
        int start_line = itemIndex;
        while (start_line > 0 && lines[start_line].rfind("Tema: ", 0) != 0) {
            start_line--;
        }

        // Find the end of the current repaso block
        int end_line = start_line;
        while (end_line < lines.size() && lines[end_line].find("--------------------------------") == string::npos) {
            end_line++;
        }
        if (end_line < lines.size()) {
            end_line++;
        }

        lines.erase(lines.begin() + start_line, lines.begin() + end_line);
    }

    ofstream outfile("repasos.txt");
    for(const auto& l : lines) {
        outfile << l << endl;
    }
    outfile.close();
    LoadRepasosToListView(hwndListView);
    MessageBox(NULL, "Repaso eliminado.", "Exito", MB_OK | MB_ICONINFORMATION);
}


LRESULT CALLBACK SpacedPracticeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hTopicEdit, hListView;

    switch (msg) {
        case WM_NOTIFY: {
            LPNMHDR lpnmh = (LPNMHDR)lParam;
            if (lpnmh->code == LVN_ITEMCHANGED && lpnmh->hwndFrom == hListView) {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                if (pnmv->uChanged & LVIF_STATE) {
                    bool isChecked = ListView_GetCheckState(hListView, pnmv->iItem);
                    UpdateRepasoStatusInFile(pnmv->iItem, isChecked);
                }
            }
            break;
        }
        case WM_CREATE: {
            HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            CreateWindow(TEXT("STATIC"), TEXT("Tema a estudiar:"), WS_VISIBLE | WS_CHILD, 20, 20, 150, 20, hwnd, NULL, NULL, NULL);
            hTopicEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 45, 280, 25, hwnd, (HMENU)IDC_TOPIC_EDIT, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Eliminar Repaso"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 320, 45, 150, 25, hwnd, (HMENU)IDC_DELETE_REPASO_BUTTON, NULL, NULL);
            CreateWindow(TEXT("BUTTON"), TEXT("Programar"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 320, 75, 150, 25, hwnd, (HMENU)IDC_SCHEDULE_BUTTON, NULL, NULL);
            
            hListView = CreateWindowEx(0, WC_LISTVIEW, TEXT(""), WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SINGLESEL, 20, 110, 450, 320, hwnd, (HMENU)IDC_REPASOS_LISTVIEW, NULL, NULL);
            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);
            
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = (LPSTR)"Descripcion del Repaso";
            lvc.cx = 425;
            ListView_InsertColumn(hListView, 0, &lvc);
            
            LoadRepasosToListView(hListView);

            CreateWindow(TEXT("BUTTON"), TEXT("Volver al Menu"), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 165, 455, 150, 40, hwnd, (HMENU)IDC_BACK_BUTTON, NULL, NULL);

            SendMessage(hTopicEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hListView, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case IDC_BACK_BUTTON: DestroyWindow(hwnd); break;
                case IDC_SCHEDULE_BUTTON: {
                    char topic[256];
                    GetWindowText(hTopicEdit, topic, 256);
                    if (strlen(topic) == 0) {
                        MessageBox(hwnd, "Por favor, ingrese un tema.", "Advertencia", MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    ofstream outfile("repasos.txt", ios_base::app);
                    outfile << "Tema: " << topic << endl;
                    auto now = chrono::system_clock::now();
                    int review_days[] = {1, 3, 7, 15, 30};
                    for (int days : review_days) {
                        auto review_time = now + chrono::hours(24 * days);
                        time_t end_time = chrono::system_clock::to_time_t(review_time);
                        char mbstr[100];
                        tm* tm_buf = localtime(&end_time);
                        strftime(mbstr, sizeof(mbstr), "%d/%m/%Y", tm_buf);
                        outfile << "  - Repaso: " << mbstr << endl;
                    }
                    outfile << "--------------------------------" << endl;
                    outfile.close();
                    LoadRepasosToListView(hListView);
                    SetWindowText(hTopicEdit, "");
                    MessageBox(hwnd, "Repaso programado y guardado.", "Exito", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                case IDC_DELETE_REPASO_BUTTON: {
                    int sel = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
                    if (sel != -1) {
                        DeleteRepaso(hListView, sel);
                    } else {
                        MessageBox(hwnd, "Por favor, seleccione un repaso para eliminar.", "Advertencia", MB_OK | MB_ICONWARNING);
                    }
                    break;
                }
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}




LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            static HWND hBtn1, hBtn2, hBtn3, hBtn4, hBtn5;
            RECT rect;
            GetClientRect(hwnd, &rect);
            int windowWidth = rect.right - rect.left;
            
            int buttonWidth = 250, buttonHeight = 50, spacing = 25;
            int totalHeight = (buttonHeight * 5) + (spacing * 4);
            int startX = (windowWidth - buttonWidth) / 2;
            int startY = ((rect.bottom - rect.top) - totalHeight) / 2;

            HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Segoe UI"));

            hBtn1 = CreateWindow(TEXT("BUTTON"), TEXT("Practica Espaciada"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, startX, startY, buttonWidth, buttonHeight, hwnd, (HMENU)IDC_SPACED_PRACTICE_BUTTON, NULL, NULL);
            hBtn2 = CreateWindow(TEXT("BUTTON"), TEXT("Recuperacion Activa"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, startX, startY + (buttonHeight + spacing), buttonWidth, buttonHeight, hwnd, (HMENU)IDC_ACTIVE_RECALL_BUTTON, NULL, NULL);
            hBtn3 = CreateWindow(TEXT("BUTTON"), TEXT("Interleaving"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, startX, startY + 2 * (buttonHeight + spacing), buttonWidth, buttonHeight, hwnd, (HMENU)IDC_INTERLEAVING_BUTTON, NULL, NULL);
            hBtn4 = CreateWindow(TEXT("BUTTON"), TEXT("Resultados"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, startX, startY + 3 * (buttonHeight + spacing), buttonWidth, buttonHeight, hwnd, (HMENU)IDC_RESULTS_BUTTON, NULL, NULL);
            hBtn5 = CreateWindow(TEXT("BUTTON"), TEXT("Salir"), WS_TABSTOP | WS_VISIBLE | WS_CHILD, startX, startY + 4 * (buttonHeight + spacing), buttonWidth, buttonHeight, hwnd, (HMENU)IDC_EXIT_BUTTON, NULL, NULL);
            
            SendMessage(hBtn1, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtn2, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtn3, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtn4, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hBtn5, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        case WM_COMMAND: {
            HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            switch (LOWORD(wParam)) {
                case IDC_SPACED_PRACTICE_BUTTON: DisplaySpacedPracticeWindow(hwnd, hInstance); break;
                case IDC_ACTIVE_RECALL_BUTTON: DisplayActiveRecallWindow(hwnd, hInstance); break;
                case IDC_INTERLEAVING_BUTTON: DisplayInterleavingWindow(hwnd, hInstance); break;
                case IDC_RESULTS_BUTTON: DisplayResultsWindow(hwnd, hInstance); break;
                case IDC_EXIT_BUTTON: DestroyWindow(hwnd); break;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    const char CLASS_NAME[] = "StudyAppWindowClass";

    HBRUSH hbrBackground = CreateSolidBrush(RGB(240, 240, 240));

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hbrBackground;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClass(&wc);

    RegisterSpacedPracticeWindow(hInstance, hbrBackground);
    RegisterActiveRecallWindow(hInstance, hbrBackground);
    RegisterPracticeWindow(hInstance, hbrBackground);
    RegisterInterleavingWindow(hInstance, hbrBackground);
    RegisterResultsWindow(hInstance, hbrBackground);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, TEXT("Study Techniques App"),
        WS_OVERLAPPEDWINDOW,
        0, 0, 
        GetSystemMetrics(SM_CXSCREEN), 
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        DeleteObject(hbrBackground);
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    CreateBackup();
    LoadConfig();
    LoadProgress();

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    DeleteObject(hbrBackground);
    return (int)msg.wParam;
}