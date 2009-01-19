/*
 * Copyright 1999-2008 University of Chicago
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.globus.usage.report.combined;

public class RLSUsageDiff {
    public static void main(String[] args) throws Exception {
        String newArgs[] = new String[args.length + 6];

        newArgs[0] = "-t";
        newArgs[1] = "rls_packets";
        newArgs[2] = "-c";
        newArgs[3] = "ip_address";
        newArgs[4] = "-r";
        newArgs[5] = "combined-rlsusagediff-report";

        for (int i = 0; i < args.length; i++) {
            newArgs[i+6] = args[i];
        }

        UsageDiff.main(newArgs);
    }
}
