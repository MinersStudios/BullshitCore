import java.io.File;
import java.nio.file.Files;
import java.nio.file.Paths;
import javax.script.ScriptEngineManager;

public class Bootstrap {
	public static void main(String[] args) {
		try {
			String script = new String(Files.readAllBytes(Paths.get("src/main/javascript/Main.js".replaceAll("/", File.separator))));
			new ScriptEngineManager().getEngineByName("nashorn").eval(script);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
