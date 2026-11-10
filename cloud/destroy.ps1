# Removes everything deploy.ps1 created (stack, thing, certificate, shadow, local secrets).
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$cfg = Get-Content (Join-Path $root 'config.json') -Raw | ConvertFrom-Json
$certDir = Join-Path $root '.certs'
$arnFile = Join-Path $certDir "$($cfg.thing_name).cert-arn.txt"
$env:AWS_DEFAULT_REGION = $cfg.region

if (Test-Path $arnFile) {
    $certArn = (Get-Content $arnFile -Raw).Trim()
    $certId = $certArn.Split('/')[-1]
    & aws iot detach-thing-principal --thing-name $cfg.thing_name --principal $certArn
    & aws iot detach-policy --policy-name "$($cfg.stack_name)-device" --target $certArn
    & aws iot update-certificate --certificate-id $certId --new-status INACTIVE
    & aws iot delete-certificate --certificate-id $certId
}
& aws iot-data delete-thing-shadow --thing-name $cfg.thing_name "$env:TEMP\shadow-delete.json" *> $null
& aws iot delete-thing --thing-name $cfg.thing_name
& aws cloudformation delete-stack --stack-name $cfg.stack_name
& aws cloudformation wait stack-delete-complete --stack-name $cfg.stack_name

Remove-Item -Recurse -Force $certDir -ErrorAction SilentlyContinue
Remove-Item -Force (Join-Path $root '..\firmware\security_hub\aws_secrets.h') -ErrorAction SilentlyContinue
Write-Host 'AWS resources and local certificates removed.'
