
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

//
// ────────────────────────────────────────────────────────────────
//  1) NETTOYAGE DE LA QUESTION
// ────────────────────────────────────────────────────────────────
//

void trim(char *str) {
    int start = 0;
    int end = strlen(str) - 1;

    while (isspace((unsigned char)str[start])) start++;
    while (end >= start && isspace((unsigned char)str[end])) end--;

    int j = 0;
    for (int i = start; i <= end; i++) {
        str[j++] = str[i];
    }
    str[j] = '\0';
}

//
// ────────────────────────────────────────────────────────────────
//  2) RECHERCHE WEB (DuckDuckGo HTML, sans clé)
// ────────────────────────────────────────────────────────────────
//

void search_web(const char *user_question) {
    char query[512];
    char command[2048];

    // Copier la question dans query
    snprintf(query, sizeof(query), "%s", user_question);

    // Remplacer espaces par +
    for (int i = 0; query[i]; i++) {
        if (query[i] == ' ') query[i] = '+';
    }

    // DuckDuckGo HTML (résultats web simples, sans clé)
    snprintf(command, sizeof(command),
        "curl -s \"https://duckduckgo.com/html/?q=%s&kl=fr-fr&ia=web\" > result.html",
        query);

    system(command);
}

//
// ────────────────────────────────────────────────────────────────
//  3) LECTURE D’UN FICHIER ENTIER EN MÉMOIRE
// ────────────────────────────────────────────────────────────────
//

char *load_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Erreur : impossible d'ouvrir %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        printf("Erreur : malloc echoue\n");
        fclose(f);
        return NULL;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';

    fclose(f);
    return buffer;
}

//
// ────────────────────────────────────────────────────────────────
//  4) EXTRACTION DE QUELQUES SNIPPETS DE LA PAGE HTML DUCKDUCKGO
// ────────────────────────────────────────────────────────────────
//
// On ne fait pas un parsing parfait, juste :
// - récupérer quelques titres et extraits
// - récupérer quelques URLs
// - construire un texte lisible pour l'IA
//

char *extract_snippets(const char *html) {
    if (!html) return NULL;

    char *snippets = malloc(12000);
    if (!snippets) return NULL;
    snippets[0] = '\0';

    const char *p = html;
    int results_count = 0;

    while ((p = strstr(p, "result__a")) && results_count < 5) {

        // URL
        const char *href_start = strstr(p, "href=\"");
        if (!href_start) break;
        href_start += 6;

        const char *href_end = strchr(href_start, '"');
        if (!href_end) break;

        char url[600];
        int url_len = href_end - href_start;
        if (url_len > 599) url_len = 599;
        strncpy(url, href_start, url_len);
        url[url_len] = '\0';

        // Titre
        const char *title_start = strchr(href_end, '>');
        if (!title_start) break;
        title_start++;

        const char *title_end = strstr(title_start, "</a>");
        if (!title_end) break;

        char title[600];
        int title_len = title_end - title_start;
        if (title_len > 599) title_len = 599;
        strncpy(title, title_start, title_len);
        title[title_len] = '\0';

        // Nettoyage du titre
        for (int i = 0; title[i]; i++) {
            if (title[i] == '<') title[i] = ' ';
        }

        // Snippet (facultatif)
        char snippet[600];
        snippet[0] = '\0';

        const char *snippet_pos = strstr(title_end, "result__snippet");
        if (snippet_pos) {
            const char *snip_start = strchr(snippet_pos, '>');
            if (snip_start) {
                snip_start++;
                const char *snip_end = strstr(snip_start, "</");
                if (snip_end) {
                    int snip_len = snip_end - snip_start;
                    if (snip_len > 599) snip_len = 599;
                    strncpy(snippet, snip_start, snip_len);
                    snippet[snip_len] = '\0';

                    for (int i = 0; snippet[i]; i++) {
                        if (snippet[i] == '<') snippet[i] = ' ';
                    }
                }
            }
        }

        // Ajout au buffer
        strcat(snippets, "Titre : ");
        strcat(snippets, title);
        strcat(snippets, "\nURL : ");
        strcat(snippets, url);

        if (strlen(snippet) > 0) {
            strcat(snippets, "\nExtrait : ");
            strcat(snippets, snippet);
        }

        strcat(snippets, "\n---\n");

        results_count++;
        p = title_end;
    }

    if (results_count == 0) {
        strcpy(snippets, "Aucune source trouvee automatiquement.");
    }

    return snippets;
}

//
// ────────────────────────────────────────────────────────────────
//  5) LECTURE DE LA CLÉ MISTRAL DANS .env
// ────────────────────────────────────────────────────────────────
//

char* load_api_key() {
    FILE *f = fopen(".env", "r");
    if (!f) {
        printf("Erreur : fichier .env introuvable\n");
        exit(1);
    }

    static char key[256];
    // On lit une ligne du type MISTRAL_KEY=xxxxxxxx
    while (fgets(key, sizeof(key), f)) {
        if (strncmp(key, "MISTRAL_KEY=", 12) == 0) {
            char *value = key + 12;
            // enlever saut de ligne final
            char *newline = strchr(value, '\n');
            if (newline) *newline = '\0';
            fclose(f);
            return value;
        }
    }

    fclose(f);
    printf("Erreur : MISTRAL_KEY introuvable dans .env\n");
    exit(1);
}

//
// ────────────────────────────────────────────────────────────────
//  6) ÉCHAPPEMENT JSON
// ────────────────────────────────────────────────────────────────
//

char* escape_json(const char *input) {
    size_t len = strlen(input);
    char *escaped = malloc(len * 4 + 1); // large buffer
    if (!escaped) return NULL;

    int j = 0;
    for (int i = 0; i < len; i++) {
        char c = input[i];
        switch (c) {
            case '\"': escaped[j++] = '\\'; escaped[j++] = '\"'; break;
            case '\\': escaped[j++] = '\\'; escaped[j++] = '\\'; break;
            case '\n': escaped[j++] = '\\'; escaped[j++] = 'n'; break;
            case '\r': escaped[j++] = '\\'; escaped[j++] = 'r'; break;
            case '\t': escaped[j++] = '\\'; escaped[j++] = 't'; break;
            default:
                escaped[j++] = c;
        }
    }
    escaped[j] = '\0';
    return escaped;
}

//
// ────────────────────────────────────────────────────────────────
//  7) APPEL À L’API MISTRAL
// ────────────────────────────────────────────────────────────────
//
void call_mistral_api(const char *question, const char *snippets) {
    const char *api_key = load_api_key();

    FILE *f = fopen("payload.json", "w");
    if (!f) {
        printf("Erreur : impossible de créer payload.json\n");
        return;
    }

    // Échapper les chaînes pour JSON
    char *escaped_question = escape_json(question);
    char *escaped_snippets = escape_json(snippets);

    // Construction du JSON propre
    fprintf(f,
        "{\n"
        "  \"model\": \"mistral-small-latest\",\n"
        "  \"messages\": [\n"
        "    {\n"
        "      \"role\": \"user\",\n"
        "      \"content\": \"Bonjour, voici la question : %s.\\n\\n"
        "Voici les extraits trouves sur le web :\\n%s\\n\\n"
        "Analyse-les et reponds STRICTEMENT dans ce format :\\n"
        "Bonjour,\\n"
        "Voici la reponse a votre question :\\n\\n"
        "Reponse : VRAI / FAUX / PAS SUR\\n"
        "Explication : ...\\n"
        "Sources :\\n"
        "- ...\\n"
        "- ...\\n\\n"
        "J'espere que cela vous aura aide, a bientot !\"\n"
        "    }\n"
        "  ]\n"
        "}\n",
        escaped_question,
        escaped_snippets
    );

    fclose(f);

    // Libération mémoire
    free(escaped_question);
    free(escaped_snippets);

    // Appel API
    char command[2048];
    snprintf(command, sizeof(command),
        "curl -s -X POST https://api.mistral.ai/v1/chat/completions "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer %s\" "
        "-d @payload.json > ai_result.json",
        api_key
    );

    system(command);

    printf("\nAnalyse IA terminee. Resultat dans ai_result.json\n");
}



//
// ────────────────────────────────────────────────────────────────
//  Afficher la reponse finale:
// ────────────────────────────────────────────────────────────────
// 
void afficher_reponse_finale() {
    char *json = load_file("ai_result.json");
    if (!json) {
        printf("Erreur : impossible de lire ai_result.json\n");
        return;
    }

    // Trouver le champ "content"
    char *start = strstr(json, "\"content\":\"");
    if (!start) {
        printf("Erreur : champ content introuvable\n");
        free(json);
        return;
    }

    start += strlen("\"content\":\"");

    // Trouver la fin du champ content
    char *end = strstr(start, "\"}");
    if (!end) {
        end = strchr(start, '"');
        if (!end) end = start + strlen(start);
    }

    int len = end - start;
    char *result = malloc(len + 1);
    strncpy(result, start, len);
    result[len] = '\0';

    // Convertir \n en vrais retours à la ligne
    for (int i = 0; result[i]; i++) {
        if (result[i] == '\\' && result[i+1] == 'n') {
            result[i] = '\n';
            memmove(&result[i+1], &result[i+2], strlen(&result[i+2]) + 1);
        }
    }

    // Affichage propre
    printf("\n==============================\n");
    printf("%s\n", result);
    printf("==============================\n\n");

    free(result);
    free(json);
}

//
// ────────────────────────────────────────────────────────────────
//  8) MAIN : CHAÎNAGE COMPLET
// ────────────────────────────────────────────────────────────────
//
int main() {
    setlocale(LC_ALL, "");
    char question[256];
    char original_question[256];

    printf("Votre question (affirmation a verifier) : ");
    if (!fgets(question, sizeof(question), stdin)) {
        printf("Erreur de lecture\n");
        return 1;
    }

    snprintf(original_question, sizeof(original_question), "%s", question);
    trim(question);

    printf("\nRecherche web sur DuckDuckGo...\n");
    search_web(question);

    char *html_data = load_file("result.html");
    if (!html_data) {
        printf("Erreur : result.html introuvable\n");
        return 1;
    }

    char *snippets = extract_snippets(html_data);

    printf("Analyse IA en cours...\n");
    call_mistral_api(original_question, snippets);

    // 🔥 C’EST ICI QUE TU DOIS AFFICHER LA RÉPONSE FINALE
    afficher_reponse_finale();

    free(html_data);
    free(snippets);

    return 0;
}
