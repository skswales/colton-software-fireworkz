@rem diff this branch with trunk
@set DIFF_DIR=%1
@shift
@rem ignore version control directories, object file directories and VS20xx project files
@rem e.g.  trunk-diff  t5  -b
diff %1 %2 %3 %4 -d -r -x .svn -x Debug* -x Release* -x "Windows Mo*" -x *.sln -x *.vc* -x .vs -x .vscode %DIFF_DIR% ..\trunk\%DIFF_DIR%