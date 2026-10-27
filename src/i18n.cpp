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
    "Catalog:",
    "Edit",
    "Catalog URL",
    "Story catalog URL (empty uses the default)",
    "Save",
    "Default",
    "Kiddo (default)",
    // Story screen strings
    "Leave Story",
    "Leave this story?\n\nYou can continue later from here.",
    "Leave",
    "Stay",
    // Inventory strings
    "Inventory",
    "New Item!",
    "Item added to inventory.",
    "Choose Item",
    "Not needed right now.",
    "Your backpack is empty.",
    // Image loading strings
    "Loading\nImage...",
    "Decode\nFailed",
    "Load\nFailed",
    // Toasts and prompts
    "Catalog updated",
    "Using default catalog URL",
    "Enter password for %s",
    // Resume strings
    "Welcome back!",
    "Continue where you left off?",
    "Continue",
    "Start over",
    // Loading strings
    "Looking for new stories...",
    "Downloading story...",
    "Stories to dream about"
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
    "Português",
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
    "Catálogo:",
    "Editar",
    "URL do Catálogo",
    "URL do catálogo (vazio usa o padrão)",
    "Salvar",
    "Padrão",
    "Kiddo (padrão)",
    // Story screen strings
    "Sair da História",
    "Sair desta história?\n\nVocê pode continuar daqui depois.",
    "Sair",
    "Ficar",
    // Inventory strings
    "Inventário",
    "Novo Item!",
    "Item adicionado ao inventário.",
    "Escolher Item",
    "Não é necessário agora.",
    "A mochila está vazia.",
    // Image loading strings
    "Carregando\nImagem...",
    "Falha na\nDecodificação",
    "Falha no\nCarregamento",
    // Toasts and prompts
    "Catálogo atualizado",
    "Usando URL padrão do catálogo",
    "Digite a senha de %s",
    // Resume strings
    "Bem-vindo de volta!",
    "Continuar de onde parou?",
    "Continuar",
    "Recomeçar",
    // Loading strings
    "Procurando novas histórias...",
    "Baixando história...",
    "Histórias para sonhar"
};

const LocalizedStrings *S()
{
    return current_language == LANG_PT ? &strings_pt : &strings_en;
}
