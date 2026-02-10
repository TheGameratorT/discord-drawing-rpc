function Component()
{
    // Constructor
}

Component.prototype.createOperations = function()
{
    // Call default implementation to actually install files
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Create Start Menu shortcut for the main GUI application only
        component.addOperation("CreateShortcut", 
            "@TargetDir@/DiscordDrawingRPC.exe", 
            "@StartMenuDir@/Discord Drawing RPC.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/DiscordDrawingRPC.exe",
            "iconId=0",
            "description=Launch Discord Drawing RPC");

        // Create Desktop shortcut (optional, user can choose)
        component.addOperation("CreateShortcut", 
            "@TargetDir@/DiscordDrawingRPC.exe", 
            "@DesktopDir@/Discord Drawing RPC.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/DiscordDrawingRPC.exe",
            "iconId=0",
            "description=Launch Discord Drawing RPC");

        // Register the main application for uninstall
        component.addOperation("RegisterFileType",
            "discorddrawingrpc",
            "@TargetDir@/DiscordDrawingRPC.exe",
            "Discord Drawing RPC Application",
            "text/plain",
            "@TargetDir@/DiscordDrawingRPC.exe,0");
    }
}

Component.prototype.createOperationsForArchive = function(archive)
{
    component.addOperation("Extract", archive, "@TargetDir@");
}
