
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

char *extraire_champ(const char *texte, const char *etiquette);
char **extraire_sources(const char *texte, int *nb);
int score_confiance(const char *reponse);
void sauvegarder_resultat_json(const char *reponse, int confiance, const char *explication, const char **sources, int nb_sources);


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

char *extraire_champ(const char *texte, const char *etiquette) {
    const char *pos = strstr(texte, etiquette);
    if (!pos) return NULL;

    // Aller après l’étiquette
    pos += strlen(etiquette);

    // Sauter les espaces, deux-points, étoiles, etc.
    while (*pos == ' ' || *pos == ':' || *pos == '*' || *pos == '\t') pos++;

    // Chercher la fin de ligne ou double saut de ligne
    const char *fin = strstr(pos, "\n\n");
    if (!fin) fin = pos + strlen(pos);

    int len = fin - pos;
    char *resultat = malloc(len + 1);
    strncpy(resultat, pos, len);
    resultat[len] = '\0';
    return resultat;
}


char **extraire_sources(const char *texte, int *nb) {
    *nb = 0;
    char **liste = malloc(10 * sizeof(char *));
    const char *p = texte;
    while ((p = strstr(p, "- "))) {
        p += 2;
        const char *fin = strchr(p, '\n');
        if (!fin) fin = p + strlen(p);
        int len = fin - p;
        char *src = malloc(len + 1);
        strncpy(src, p, len);
        src[len] = '\0';
        liste[*nb] = src;
        (*nb)++;
        if (*nb >= 10) break;
        p = fin;
    }
    return liste;
}

int score_confiance(const char *reponse) {
    if (strstr(reponse, "VRAI")) return 95;
    if (strstr(reponse, "FAUX")) return 5;
    return 50;
}

void sauvegarder_resultat_json(const char *reponse, int confiance, const char *explication, const char **sources, int nb_sources) {
    FILE *f = fopen("ai_result.json", "w");
    if (!f) return;

    fprintf(f, "{\n");
    fprintf(f, "  \"answer\": \"%s\",\n", escape_json(reponse));
    fprintf(f, "  \"confidence\": %d,\n", confiance);
    fprintf(f, "  \"explanation\": \"%s\",\n", escape_json(explication));
    fprintf(f, "  \"sources\": [\n");
    for (int i = 0; i < nb_sources; i++) {
        fprintf(f, "    \"%s\"%s\n", escape_json(sources[i]), (i < nb_sources - 1) ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
}


// ────────────────────────────────────────────────────────────────
//  8) MAIN : CHAÎNAGE COMPLET
// ────────────────────────────────────────────────────────────────
//

int main() {
    setlocale(LC_ALL, "");

    // Lire la question
    char *question_file = load_file("question.txt");
    if (!question_file) return 1;

    char question[512];
    snprintf(question, sizeof(question), "%s", question_file);
    trim(question);

    // Recherche web
    search_web(question);
    char *html = load_file("result.html");
    if (!html) return 1;

    char *snippets = extract_snippets(html);

    // Appel API
    call_mistral_api(question, snippets);

    // Lire la réponse brute
    char *json = load_file("ai_result.json");
    if (!json) return 1;

    char *start = strstr(json, "\"content\":\"");
    if (!start) return 1;
    start += strlen("\"content\":\"");

    char *end = strstr(start, "\"}");
    if (!end) end = start + strlen(start);

    int len = end - start;
    char *content = malloc(len + 1);
    strncpy(content, start, len);
    content[len] = '\0';

    // Affichage du contenu brut pour debug
printf("\n--- CONTENU BRUT DE L'IA ---\n%s\n----------------------------\n", content);


    // Nettoyage des \n
    for (int i = 0; content[i]; i++) {
        if (content[i] == '\\' && content[i+1] == 'n') {
            content[i] = '\n';
            memmove(&content[i+1], &content[i+2], strlen(&content[i+2]) + 1);
        }
    }

    // Extraction des champs
    char *reponse = extraire_champ(content, "Réponse");
    char *explication = extraire_champ(content, "Explication");
    int confiance = score_confiance(reponse);
    int nb_sources = 0;
    char **sources = extraire_sources(content, &nb_sources);

    // Sauvegarde finale
   sauvegarder_resultat_json(reponse, confiance, explication, (const char **)sources, nb_sources);


    // Libération mémoire
    free(question_file);
    free(html);
    free(snippets);
    free(json);
    free(content);
    free(reponse);
    free(explication);
    for (int i = 0; i < nb_sources; i++) free(sources[i]);
    free(sources);

    return 0;

}
