/*#include <windows.h>
#include <stdio.h>

HWND hInput, hOutput;

void lancerAnalyse() {
    char question[512];
    char buffer[20000] = "";   // ← AJOUTER ICI
    char line[512];

    GetWindowTextA(hInput, question, sizeof(question));

    FILE *fq = fopen("question.txt", "w");
    if (!fq) return;
    fprintf(fq, "%s\n", question);
    fclose(fq);

    int status = system("agent.exe < question.txt > output.txt");
    if (status != 0) {
        MessageBox(NULL, "Erreur : agent.exe n'a pas pu s'exécuter correctement.", "Erreur", MB_ICONERROR);
        return;
    }

    FILE *fr = fopen("output.txt", "r");
    if (!fr) return;

    while (fgets(line, sizeof(line), fr)) {
        strcat(buffer, line);
    }
    fclose(fr);

    if (strlen(buffer) < 10) {
        SetWindowTextA(hOutput, "Aucune réponse générée.");
        return;
    }

    SetWindowTextA(hOutput, buffer);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                lancerAnalyse();
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "AnalyseurIA";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "AnalyseurIA", "Analyseur IA",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInstance, NULL
    );

    CreateWindow(
        "STATIC", "Votre question :", 
        WS_VISIBLE | WS_CHILD,
        10, 10, 200, 20,
        hwnd, NULL, hInstance, NULL
    );

    hInput = CreateWindow(
        "EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        10, 35, 660, 25,
        hwnd, NULL, hInstance, NULL
    );

    CreateWindow(
        "BUTTON", "Lancer l'analyse",
        WS_VISIBLE | WS_CHILD,
        10, 70, 150, 30,
        hwnd, (HMENU)1, hInstance, NULL
    );

    hOutput = CreateWindow(
        "EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
        10, 110, 660, 340,
        hwnd, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
*/
/*
#include <windows.h>
#include <stdio.h>

HWND hInput, hOutput;
char* lire_reponse_ia() {
    FILE *f = fopen("ai_result.json", "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *json = malloc(size + 1);
    fread(json, 1, size, f);
    json[size] = '\0';
    fclose(f);

    // Cherche "content"
    char *start = strstr(json, "\"content\":\"");
    if (!start) {
        free(json);
        return NULL;
    }

    start += strlen("\"content\":\"");
    char *end = strchr(start, '"');
    if (!end) {
        free(json);
        return NULL;
    }

    *end = '\0';

    // Remplace \n par vrais retours à la ligne
    for (char *p = start; *p; p++) {
        if (*p == '\\' && *(p+1) == 'n') {
            *p = '\n';
            memmove(p+1, p+2, strlen(p+2)+1);
        }
    }

    char *result = _strdup(start); // copie propre
    free(json);
    return result;
}

void lancerAnalyse() {
    char question[512];
    GetWindowTextA(hInput, question, sizeof(question));

    // Écrire la question dans un fichier
    FILE *fq = fopen("question.txt", "w");
    if (!fq) return;
    fprintf(fq, "%s\n", question);
    fclose(fq);

    // Lancer agent.exe avec redirection
    int status = system("agent.exe < question.txt > output.txt");
    if (status != 0) {
        MessageBox(NULL, "Erreur : agent.exe n'a pas pu s'exécuter.", "Erreur", MB_ICONERROR);
        return;
    }

    // Lire la réponse
  char *reponse = lire_reponse_ia();
if (!reponse) {
    SetWindowTextA(hOutput, "Erreur : champ content introuvable dans ai_result.json");
    return;
}

SetWindowTextA(hOutput, reponse);
free(reponse);

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                lancerAnalyse();
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "AnalyseurIA";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "AnalyseurIA", "Analyseur IA",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInstance, NULL
    );

    CreateWindow("STATIC", "Votre question :", WS_VISIBLE | WS_CHILD, 10, 10, 200, 20, hwnd, NULL, hInstance, NULL);

    hInput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 35, 660, 25, hwnd, NULL, hInstance, NULL);

    CreateWindow("BUTTON", "Lancer l'analyse", WS_VISIBLE | WS_CHILD, 10, 70, 150, 30, hwnd, (HMENU)1, hInstance, NULL);

    hOutput = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                           10, 110, 660, 340, hwnd, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

*/