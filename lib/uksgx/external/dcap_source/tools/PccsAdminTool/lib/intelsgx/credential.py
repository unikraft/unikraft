import keyring
import getpass

class Credentials:
    APPNAME = 'PccsAdmin'
    KEY_ADMINTOKEN = 'ADMIN_TOKEN'
    KEY_PCS_APIKEY = 'PCS_API_KEY'

    def get_admin_token(self):
        admin_token = ""
        try:
            admin_token = keyring.get_password(self.APPNAME, self.KEY_ADMINTOKEN)
        except keyring.errors.KeyringError as ke:
            admin_token = ""
        
        while admin_token is None or admin_token == '':
            admin_token = getpass.getpass(prompt="Please input your administrator password for PCCS service:")
            # prompt saving password
            if admin_token != "":
                save_passwd = input("Would you like to remember password in OS keyring? (y/n)")
                if save_passwd.lower() == 'y':
                    self.set_admin_token(admin_token)

        return admin_token

    def set_admin_token(self, token):
        try:
            keyring.set_password(self.APPNAME, self.KEY_ADMINTOKEN, token)
        except keyring.errors.PasswordSetError as ke:
            print("Failed to store admin token.")
            return False
        return True

    def get_pcs_api_key(self):
        pcs_api_key = ""
        try:
            pcs_api_key = keyring.get_password(self.APPNAME, self.KEY_PCS_APIKEY)
        except keyring.errors.KeyringError as ke:
            pcs_api_key = ""
        
        while pcs_api_key is None or pcs_api_key == '':
            pcs_api_key = getpass.getpass(prompt="Please input ApiKey for Intel PCS:")
            # prompt saving password
            if pcs_api_key != "":
                save_passwd = input("Would you like to remember Intel PCS ApiKey in OS keyring? (y/n)")
                if save_passwd.lower() == 'y':
                    self.set_pcs_api_key(pcs_api_key)

        return pcs_api_key

    def set_pcs_api_key(self, apikey):
        try:
            keyring.set_password(self.APPNAME, self.KEY_PCS_APIKEY, apikey)
        except keyring.errors.PasswordSetError as ke:
            print("Failed to store PCS API key.")
            return False
        return True
