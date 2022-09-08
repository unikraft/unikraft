$csharp = @"
 public class Reader
    {
        public static string ReadCDefine(string content, string key)
        {
            foreach (string line in System.IO.File.ReadAllLines(content))
            {
                string lineT = line.Trim();
                string def = "#define " + key + " ";
                if (lineT.StartsWith(def))
                {
                    return line.Substring(def.Length).Trim().Trim('"');
                }
            }
            return null;
        }
    }
"@
Add-Type -TypeDefinition "$csharp"

$version = [Reader]::ReadCDefine("$PSScriptRoot\..\..\..\QuoteGeneration\common\inc\internal\se_version.h", "STRPRODUCTVER")
Write-Output $version
