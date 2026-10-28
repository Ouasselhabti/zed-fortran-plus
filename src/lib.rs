use zed_extension_api::{self as zed, settings::LspSettings, LanguageServerId, Result};

struct FortranExtension;

impl zed::Extension for FortranExtension {
    fn new() -> Self {
        FortranExtension
    }

    /// Runs `fortls` from PATH, unless `lsp.fortls.binary` in the settings
    /// gives another path or other arguments.
    fn language_server_command(
        &mut self,
        language_server_id: &LanguageServerId,
        worktree: &zed::Worktree,
    ) -> Result<zed::Command> {
        let binary = LspSettings::for_worktree(language_server_id.as_ref(), worktree)
            .ok()
            .and_then(|settings| settings.binary);
        let command = binary
            .as_ref()
            .and_then(|binary| binary.path.clone())
            .or_else(|| worktree.which("fortls"))
            .ok_or_else(|| {
                "fortls not found: install it (pip install fortls) or set lsp.fortls.binary.path"
                    .to_string()
            })?;
        let args = binary
            .and_then(|binary| binary.arguments)
            .unwrap_or_else(|| vec!["--lowercase_intrinsics".to_string()]);
        Ok(zed::Command {
            command,
            args,
            env: worktree.shell_env(),
        })
    }
}

zed::register_extension!(FortranExtension);
