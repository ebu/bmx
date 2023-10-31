;;; dap-templates.el - Dap Templates

(defvar dolby-muxer-workspace-dir)
(setq dolby-muxer-workspace-dir (projectile-project-root))

(dap-register-debug-template
  "raw2bmx [gdb]"
  (list :type "gdb"
        :request "launch"
        :name "raw2bmx [gdb]"
        :target "./build/apps/raw2bmx/raw2bmx"
        :cwd dolby-muxer-workspace-dir))
