################################################################
## This scrypt gets size of AWE and Large-Page allocated memory
## Scrypt parse RMP file (output of RamMap.exe)
## https://docs.microsoft.com/en-us/sysinternals/downloads/rammap
################################################################
$script_start = Get-Date

# path to temporary file
$path2RmpFile = [System.IO.Path]::GetTempFileName()

# funnction for get path to work folder (where placed rammap64.exe)
function Get-ScriptDirectory
{
  $Invocation = (Get-Variable MyInvocation -Scope 1).Value
  Split-Path $Invocation.MyCommand.Path
}

# run and wait RamMap.
# $ramMap = 'rammap.exe'
$ramMap = [io.path]::combine( (Get-ScriptDirectory), 'rammap64.exe')
Start-Process $ramMap -Verb runAs -ArgumentList $path2RmpFile -Wait

$reader = [System.IO.File]::OpenText($path2RmpFile)
$xmlStr = '<UseCounts></UseCounts>'
while($null -ne ($line = $reader.ReadLine())) {
    if($line.StartsWith('<UseCounts>')) {
        $xmlStr = $line
        break
    }
    
}

# parce RamMap output file as XML
$xml = [xml]($xmlStr)

if ($xml.UseCounts.Length -lt 256) {
    throw [System.IO.IOException] "cannot parse $path2RmpFile"
}

function Convert-hex2Bin ([string]$hex) {
    <#
        .DESCRIPTION
        Converts hexadecimal string to byte array

        .PARAMETER hex
        Hexadecimal string. E.g. 'FF453400000000000'
    #>

    $numChars = $hex.Length
    $bytes = New-Object byte[] -ArgumentList ( $numChars / 2 )
    for($i=0; $i -lt $numChars; $i+=2) {
        $bytes[$i/2] = [Convert]::ToByte($hex.Substring($i, 2), 16)
    }
    return $bytes
}

$bytes = Convert-hex2Bin $xml.UseCounts

$arrCouts = for($i=0; $i -lt $bytes.Length; $i+=8) {
    [BitConverter]::ToUInt64($bytes, $i) * 4096 / 1024
}

# Write-Host $arrCouts

$awe = $arrCouts[9]
$largePages = $arrCouts[13]
"Taken time: {0}" -f ((Get-Date) - $script_start)
"Memory size in KBytes`n AWE: {0, 2}`n Large-Page: {1, 2}" -f $awe, $largePages
