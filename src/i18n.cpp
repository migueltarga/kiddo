/**
 * @file i18n.cpp
 * @brief Internationalization (i18n) system
 *
 * This module provides localized strings for the application,
 * supporting English and Portuguese languages. It contains
 * all user-facing text strings organized by language.
 */

#include "i18n.h"

/** @brief Current application language */
Language current_language = LANG_EN;

/** @brief English localized strings */
static const LocalizedStrings strings_en = {
    "Settings", "Brightness:", "Language:", "Close",
    "Reset Wi-Fi", "Reset",
    "Are you sure you want to reset Wi-Fi credentials?\n\nReconnect to SSID Kiddo to reconfigure.",
    "Cancel",
    "Library", "Story Library", "Back to Library", "The End",
    "No stories found.",
    "Next",
    "Font:",
    "Loading...",
    "Clear Cache",
    "Delete",
    "Delete all downloaded stories?",
    "Online Mode",
    "WiFi reset. Rebooting...",
    "Small",
    "Normal",
    "Large",
    "English",
    "Portuguese",
    "Download failed",
    "Retry online fetch",
    "No online catalog",
    "Stories will appear here when downloaded.",
    "Enable Online Mode in Settings\nto download stories.",
    "Cache cleared successfully.",
    "Are you sure you want to reset the WiFi settings?\n\nYou will need to reconfigure the connection.",
    // WiFi SSID screen strings
    "WiFi Networks",
    "Scan for Networks",
    "Scanning...",
    "Scan Again",
    "Tap 'Scan for Networks' to find available WiFi networks",
    "No networks found. Try scanning again.",
    "Found %d networks:",
    "Strong",
    "Good", 
    "Weak",
    "Open",
    "Secured",
    "WiFi Password",
    "Password",
    "Connect",
    "Connecting to %s...",
    "Connected successfully!",
    "Connection failed. Check password and try again.",
    "Connecting to %s... (%ds)",
    "Catalog URL:",
    "Edit",
    "Catalog URL",
    "Enter the URL for your story catalog:\n\n(Leave empty to use default)",
    "Save",
    "Default",
    // Story screen strings
    "Leave Story",
    "Are you sure you want to leave this story?\n\nYour progress will be lost.",
    "Leave",
    "Stay",
    // Inventory strings
    "Inventory",
    "New Item!",
    "You found a new item!",
    "Item added to inventory.",
    "Choose Item",
    // Image loading strings
    "Loading\nImage...",
    "Decode\nFailed",
    "Load\nFailed"
};

static const LocalizedStrings strings_pt = {
    "Opções", "Brilho:", "Idioma:", "Fechar",
    "Resetar Wi-Fi", "Resetar",
    "Tem certeza que deseja trocar as credenciais do Wi-Fi?\n\nReconecte ao SSID Kiddo para reconfigurar.",
    "Cancelar",
    "Biblioteca", "Biblioteca", "Voltar à Biblioteca", "Fim",
    "Nenhuma história encontrada.",
    "Próximo",
    "Fonte:",
    "Carregando...",
    "Limpar Cache",
    "Excluir",
    "Excluir todas as histórias baixadas?",
    "Modo Online",
    "WiFi resetado. Reiniciando...",
    "Pequena",
    "Normal",
    "Grande",
    "Ingles",
    "Portugues",
    "Falha no download",
    "Tentar buscar online novamente",
    "Nenhum catálogo online",
    "As histórias aparecerão aqui quando baixadas.",
    "Ative o Modo Online nas Configurações\npara baixar histórias.",
    "Cache limpo com sucesso.",
    "Tem certeza que deseja resetar as configurações de WiFi?\n\nVocê precisará reconfigurar a conexão.",
    // WiFi SSID screen strings
    "Redes WiFi",
    "Buscar Redes",
    "Buscando...",
    "Buscar Novamente",
    "Toque em 'Buscar Redes' para encontrar redes WiFi disponíveis",
    "Nenhuma rede encontrada. Tente buscar novamente.",
    "Encontradas %d redes:",
    "Forte",
    "Boa",
    "Fraca",
    "Aberta",
    "Protegida",
    "Senha WiFi",
    "Senha",
    "Conectar",
    "Conectando a %s...",
    "Conectado com sucesso!",
    "Falha na conexão. Verifique a senha e tente novamente.",
    "Conectando a %s... (%ds)",
    "URL do Catálogo:",
    "Editar",
    "URL do Catálogo",
    "Digite a URL para seu catálogo de histórias:\n\n(Deixe vazio para usar o padrão)",
    "Salvar",
    "Padrão",
    // Story screen strings
    "Sair da História",
    "Tem certeza que deseja sair desta história?\n\nSeu progresso será perdido.",
    "Sair",
    "Ficar",
    // Inventory strings
    "Inventário",
    "Novo Item!",
    "Você encontrou um novo item!",
    "Item adicionado ao inventário.",
    "Escolher Item",
    // Image loading strings
    "Carregando\nImagem...",
    "Falha na\nDecodificação",
    "Falha no\nCarregamento"
};

const LocalizedStrings *S()
{
    return current_language == LANG_PT ? &strings_pt : &strings_en;
}
