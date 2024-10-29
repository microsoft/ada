import argparse
import json
import os
import platform
import subprocess


storage_roles = [
    "Reader",
    "Storage Blob Data Reader",
    "Storage Blob Data Contributor",
    "Storage Queue Data Reader",
    "Storage Queue Data Contributor",
    "Storage Table Data Reader",
    "Storage Table Data Contributor",
    "Storage File Data Privileged Reader",
]


def get_storage_scope(subscription, resource_group, storage_account):
    return (
        f"/subscriptions/{subscription}/resourceGroups/{resource_group}/providers/"
        f"Microsoft.Storage/storageAccounts/{storage_account}"
    )


def get_keyvault_scope(subscription, resource_group, key_vault):
    return (
        f"/subscriptions/{subscription}/resourceGroups/{resource_group}/providers/"
        f"Microsoft.KeyVault/vaults/{key_vault}"
    )


def find_az_cmd():
    for path in os.environ["PATH"].split(os.pathsep):
        az = os.path.join(path, "az.cmd")
        if os.path.exists(az):
            return az
    return None


def add_role_assignment(az, scope, user, role, resource):
    cmd = f'"{az}" role assignment create --assignee "{user}" --role "{role}" --scope "{scope}"'
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error: {result.stderr}")
        return
    json.loads(result.stdout)  # make sure result is valid json.
    print(f"Role '{role}' added for identity '{user}' on {resource}")


def parse_command_line():
    parser = argparse.ArgumentParser(
        description="Add user roles to our Azure storage account and key vault."
        + " The user should be an SC-ALT account."
    )
    parser.add_argument("--subscription", "-s", help="The subscription to use", required=True)
    parser.add_argument("--resource_group", "-g", help="The resource group to use", required=True)
    parser.add_argument("--storage_account", "-a", help="The storage account to use", required=True)
    return parser.parse_args()


def main():
    az = find_az_cmd()
    if az is None:
        print("Error: az.cmd not found in PATH")
        return

    args = parse_command_line()

    storage_account = args.storage_account
    storage_scope = get_storage_scope(args.subscription, args.resource_group, storage_account)
    machine = platform.node()
    for role in storage_roles:
        add_role_assignment(az, storage_scope, machine, role, storage_account)


if __name__ == "__main__":
    print(platform.node())
    # main()
