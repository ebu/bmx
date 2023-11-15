;;; dap-templates.el - Dap Templates

(defvar dolby-muxer-workspace-dir)
(setq dolby-muxer-workspace-dir (projectile-project-root))

(dap-register-debug-template
  "raw2bmx-isxd-only"
  (list :type "gdb"
        :request "launch"
        :name "raw2bmx-isxd-only"
        :target "./build/apps/raw2bmx/raw2bmx"
        :arguments (concat "-o "
                           "/var/run/media/markus/markus01/dolby/repofiles/isxdonly.mxf "
                           "--isxd "
                           "/mnt/disk2_win/Dolby/CanalP/repo_intermediate_25fps/dv/dv_25_4.0.2.isxd")
        :cwd dolby-muxer-workspace-dir))
